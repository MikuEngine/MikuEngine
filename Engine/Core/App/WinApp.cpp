#include "pch.h"
#include "WinApp.h"

#include <DirectXColors.h>

#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "Common/Utility/Profiling.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/App/ConfigLoader.h"
#include "Core/System/ProjectSettings.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/ScriptSystem.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/System/CameraSystem.h"
#include "Editor/EditorManager.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace engine
{
    struct Resolution
    {
        int width;
        int height;

        bool operator==(const Resolution& other) const
        {
            return width == other.width && height == other.height;
        }

        bool operator<(const Resolution& other) const
        {
            return width < other.width && height < other.height;
        }

        bool operator<=(const Resolution& other) const
        {
            return width <= other.width && height <= other.height;
        }
    };

    constexpr std::array<Resolution, 4> g_supportedResolutions
    {
        Resolution{ 1280, 720 },
        Resolution{ 1920, 1080 },
        Resolution{ 2560, 1440 },
        Resolution{ 3840, 2160 }
    };

    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    WinApp::WinApp(const std::filesystem::path& settingFilePath, const WindowSettings& defaultSetting)
        : m_settingFilePath{ settingFilePath },
        m_settings{ defaultSetting },
        m_screenWidth{ GetSystemMetrics(SM_CXSCREEN) },
        m_screenHeight{ GetSystemMetrics(SM_CYSCREEN) }
    {
        ValidateSettings();

        HR_CHECK(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    }

    WinApp::~WinApp()
    {
        CoUninitialize();
    }
    
    void WinApp::Initialize()
    {
        SetWindowMode(m_settings.isFullscreen);

        m_hInstance = GetModuleHandleW(nullptr);
        if (m_hCursor == nullptr)
        {
            m_hCursor = LoadCursorW(nullptr, IDC_ARROW);
        }

        auto className = ToWideChar(m_className);
        auto windowName = ToWideChar(m_windowName);

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = m_classStyle;
        wc.lpfnWndProc = engine::WindowProc;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = className.c_str();
        wc.hCursor = m_hCursor;
        wc.hIcon = m_hIcon;
        wc.hIconSm = m_hIconSmall;

        FATAL_CHECK(RegisterClassExW(&wc), "RegisterClassExW failed");

        RECT rc{};
        if (m_settings.isFullscreen)
        {
            rc.right = m_screenWidth;
            rc.bottom = m_screenHeight;
        }
        else
        {
            rc.right = m_settings.resolutionWidth;
            rc.bottom = m_settings.resolutionHeight;
        }

        AdjustWindowRect(&rc, m_windowStyle, FALSE);

        int actualWidth = rc.right - rc.left;
        int actualHeight = rc.bottom - rc.top;

        m_x = (m_screenWidth - actualWidth) / 2 < 0 ? 0 : (m_screenWidth - actualWidth) / 2;
        m_y = (m_screenHeight - actualHeight) / 2 < 0 ? 0 : (m_screenHeight - actualHeight) / 2;

        m_hWnd = CreateWindowExW(
            0,
            className.c_str(),
            windowName.c_str(),
            m_windowStyle,
            m_x,
            m_y,
            actualWidth,
            actualHeight,
            nullptr,
            nullptr,
            m_hInstance,
            this
        );

        FATAL_CHECK(m_hWnd != nullptr, "CreateWindowExW failed");
        
        ShowWindow(m_hWnd, SW_SHOW);
        UpdateWindow(m_hWnd);

        GraphicsDevice::Get().Initialize(
            m_hWnd,
            static_cast<UINT>(m_settings.resolutionWidth),
            static_cast<UINT>(m_settings.resolutionHeight),
            static_cast<UINT>(m_screenWidth),
            static_cast<UINT>(m_screenHeight),
            m_settings.isFullscreen,
            m_settings.useVsync);

        Input::Initialize(m_hWnd);

        Input::SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);

        // Calculate Input Coordinate Transform
        float targetAspectRatio = static_cast<float>(m_settings.resolutionWidth) / static_cast<float>(m_settings.resolutionHeight);
        float screenAspectRatio = static_cast<float>(m_screenWidth) / static_cast<float>(m_screenHeight);

        float viewWidth = static_cast<float>(m_screenWidth);
        float viewHeight = static_cast<float>(m_screenHeight);
        float viewX = 0.0f;
        float viewY = 0.0f;

        if (m_settings.isFullscreen)
        {
            if (screenAspectRatio > targetAspectRatio)
            {
                viewWidth = m_screenHeight * targetAspectRatio;
                viewX = (m_screenWidth - viewWidth) * 0.5f;
            }
            else
            {
                viewHeight = m_screenWidth / targetAspectRatio;
                viewY = (m_screenHeight - viewHeight) * 0.5f;
            }
        }
        else
        {
            viewWidth = static_cast<float>(m_settings.resolutionWidth);
            viewHeight = static_cast<float>(m_settings.resolutionHeight);
        }

        float scaleX = viewWidth / static_cast<float>(m_settings.resolutionWidth);
        float scaleY = viewHeight / static_cast<float>(m_settings.resolutionHeight);

        Input::SetCoordinateTransform(viewX, viewY, scaleX, scaleY);

        IMGUI_CHECKVERSION();

        ImGui::CreateContext();

        ImGui_ImplWin32_Init(m_hWnd);
        ImGui_ImplDX11_Init(GraphicsDevice::Get().GetDevice().Get(), GraphicsDevice::Get().GetDeviceContext().Get());

        AssetManager::Get().Initialize();
        ResourceManager::Get().Initialize();
        SceneManager::Get().Initialize();

#ifdef _DEBUG
        EditorManager::Get().Initialize();
#else
        ProjectSettings settings;
        settings.Load();
        
        if (!settings.sceneList.empty())
        {
            SceneManager::Get().ChangeScene(settings.sceneList[0]);
        }
        else
        {
            FATAL_CHECK(false, "프로젝트 세팅 파일 오류");
        }
#endif // _DEBUG
        
    }

    void WinApp::Shutdown()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        SceneManager::Get().Shutdown();
        SystemManager::Get().Shutdown();
        ResourceManager::Get().Cleanup();
        GraphicsDevice::Get().Shutdown();
    }

    void WinApp::Run()
    {
        MSG msg{};

        while (true)
        {
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    break;
                }

                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
            {
                Update();
                Render();
            }
        }
    }

    void WinApp::Update()
    {
        Profiling::UpdateFPS(true);
        Time::Update();
        Input::Update();

#ifdef _DEBUG
        switch (EditorManager::Get().GetEditorState())
        {
        case EditorState::Edit:
            SceneManager::Get().CheckSceneChanged();
            SceneManager::Get().ProcessPendingAdds(false);

            EditorManager::Get().Update();
            break;

        case EditorState::Play:
            SceneManager::Get().CheckSceneChanged();
            SceneManager::Get().ProcessPendingAdds(true);
            SceneManager::Get().ProcessPendingKills();

            SystemManager::Get().GetScriptSystem().CallStart();
            SystemManager::Get().GetScriptSystem().CallUpdate();

            SystemManager::Get().GetCameraSystem().Update();
            break;

        case EditorState::Pause:
            break;
        }
#else
        SceneManager::Get().CheckSceneChanged();
        SceneManager::Get().ProcessPendingAdds(true);
        SceneManager::Get().ProcessPendingKills();

        SystemManager::Get().GetScriptSystem().CallStart();
        SystemManager::Get().GetScriptSystem().CallUpdate();
        SystemManager::Get().GetCameraSystem().Update();
#endif // _DEBUG
    }

    void WinApp::Render()
    {
        SystemManager::Get().GetRenderSystem().Render();

#ifdef _DEBUG
        EditorManager::Get().Render();
#endif //_DEBUG

        GraphicsDevice::Get().EndDraw();

        SystemManager::Get().GetTransformSystem().UnmarkDirtyThisFrame();
    }

    LRESULT WinApp::MessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        {
            return 0;
        }

        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_ACTIVATEAPP:
            DirectX::Keyboard::ProcessMessage(uMsg, wParam, lParam);
            DirectX::Mouse::ProcessMessage(uMsg, wParam, lParam);
            break;

        case WM_INPUT:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
            DirectX::Mouse::ProcessMessage(uMsg, wParam, lParam);
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            DirectX::Keyboard::ProcessMessage(uMsg, wParam, lParam);
            break;

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }

        return 0;
    }

    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WinApp* winApp = nullptr;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            winApp = reinterpret_cast<WinApp*>(cs->lpCreateParams);

            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(winApp));
        }
        else
        {
            winApp = reinterpret_cast<WinApp*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        }


        if (winApp != nullptr)
        {
            return winApp->MessageProc(hWnd, uMsg, wParam, lParam);
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    void WinApp::ValidateSettings()
    {
        ConfigLoader::Load(m_settingFilePath, m_settings);

        Resolution currentRes{ m_settings.resolutionWidth, m_settings.resolutionHeight };
        Resolution screenRes{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

        bool isValid = false;
        for (const auto& supported : g_supportedResolutions)
        {
            if (supported == currentRes)
            {
                if (m_settings.isFullscreen)
                {
                    if (supported <= screenRes)
                    {
                        isValid = true;
                    }
                }
                else
                {
                    if (supported < screenRes)
                    {
                        isValid = true;
                    }
                }

                break;
            }
        }

        if (!isValid)
        {
            currentRes = g_supportedResolutions[0];

            for (auto it = g_supportedResolutions.rbegin(); it != g_supportedResolutions.rend(); ++it)
            {
                const auto& supported = *it;

                if (m_settings.isFullscreen)
                {
                    if (supported <= screenRes)
                    {
                        currentRes = supported;
                        break;
                    }
                }
                else
                {
                    if (supported < screenRes)
                    {
                        currentRes = supported;
                        break;
                    }
                }
            }
        }

        m_settings.resolutionWidth = currentRes.width;
        m_settings.resolutionHeight = currentRes.height;

        m_settings.supportedResolutions.clear();
        for (const auto& res : g_supportedResolutions)
        {
            m_settings.supportedResolutions.push_back(std::format("{}x{}", res.width, res.height));
        }

        ConfigLoader::Save(m_settingFilePath, m_settings);
    }

    void WinApp::SetWindowMode(bool isFullscreen)
    {
        if (isFullscreen)
        {
            m_windowStyle = WS_POPUP | WS_VISIBLE;
        }
        else
        {
            m_windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        }
    }

    void WinApp::SetResolution(int width, int height, bool isFullscreen)
    {
        if (width == 0)
        {
            width = m_settings.resolutionWidth;
        }
        if (height == 0)
        {
            height = m_settings.resolutionHeight;
        }

        SetWindowMode(isFullscreen);
        SetWindowLongPtrW(m_hWnd, GWL_STYLE, m_windowStyle);

        RECT rc{};
        if (isFullscreen)
        {
            rc = { 0, 0, m_screenWidth, m_screenHeight };
        }
        else
        {
            rc = { 0, 0, width, height };
            AdjustWindowRect(&rc, m_windowStyle, FALSE);
        }

        int actualW = rc.right - rc.left;
        int actualH = rc.bottom - rc.top;

        int x = (m_screenWidth - actualW) / 2;
        int y = (m_screenHeight - actualH) / 2;
        if (x < 0)
        {
            x = 0;
        }
        if (y < 0)
        {
            y = 0;
        }

        m_settings.isFullscreen = isFullscreen;
        
        SetWindowPos(m_hWnd, HWND_TOP, x, y, actualW, actualH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        GraphicsDevice::Get().Resize(
            m_settings.resolutionWidth,
            m_settings.resolutionHeight,
            m_screenWidth,
            m_screenHeight,
            isFullscreen
        );

        // Calculate Input Coordinate Transform
        float targetAspectRatio = static_cast<float>(m_settings.resolutionWidth) / static_cast<float>(m_settings.resolutionHeight);
        float screenAspectRatio = static_cast<float>(m_screenWidth) / static_cast<float>(m_screenHeight);

        float viewWidth = static_cast<float>(m_screenWidth);
        float viewHeight = static_cast<float>(m_screenHeight);
        float viewX = 0.0f;
        float viewY = 0.0f;

        if (isFullscreen)
        {
            if (screenAspectRatio > targetAspectRatio)
            {
                viewWidth = m_screenHeight * targetAspectRatio;
                viewX = (m_screenWidth - viewWidth) * 0.5f;
            }
            else
            {
                viewHeight = m_screenWidth / targetAspectRatio;
                viewY = (m_screenHeight - viewHeight) * 0.5f;
            }
        }
        else
        {
            viewWidth = static_cast<float>(m_settings.resolutionWidth);
            viewHeight = static_cast<float>(m_settings.resolutionHeight);
        }

        float scaleX = viewWidth / static_cast<float>(m_settings.resolutionWidth);
        float scaleY = viewHeight / static_cast<float>(m_settings.resolutionHeight);

        Input::SetCoordinateTransform(viewX, viewY, scaleX, scaleY);
    }
}
