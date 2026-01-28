#include "EnginePCH.h"
#include "WinApp.h"

#include <DirectXColors.h>

#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "Common/Utility/Profiling.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/App/UserSettingsLoader.h"
#include "Core/System/ProjectSettings.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/PreloadManager.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/ScriptSystem.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/System/AnimatorSystem.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionSystem.h"
#include "Framework/System/UIEventSystem.h"
#include "Framework/System/SoundSystem.h"
#include "Framework/System/ParticleSystem.h"
#include "Framework/System/PathfindingSystem.h"
#include "Editor/EditorManager.h"
#include "Framework/Object/Component/Pathfinding/PathfindingDebugRenderer.h"
#include "Framework/Physics/PhysicsDebugRenderer.h"
#include "Framework/Object/Component/Light/LightDebugRenderer.h"
#include "Core/System/VirtualFileSystem.h"

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

    WinApp::WinApp(const std::filesystem::path& settingFilePath, const UserSettings& defaultSetting)
        : m_settingFilePath{ settingFilePath },
        m_userSettings{ defaultSetting },
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
        auto& ws = m_userSettings.window;

        SetWindowMode(ws.isFullscreen);

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
        if (ws.isFullscreen)
        {
            rc.right = m_screenWidth;
            rc.bottom = m_screenHeight;
        }
        else
        {
            rc.right = ws.resolutionWidth;
            rc.bottom = ws.resolutionHeight;
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
            static_cast<UINT>(ws.resolutionWidth),
            static_cast<UINT>(ws.resolutionHeight),
            static_cast<UINT>(m_screenWidth),
            static_cast<UINT>(m_screenHeight),
            ws.isFullscreen,
            ws.useVsync);

        Input::Initialize(m_hWnd);

        Input::SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);

        UpdateViewportTransformData();

        IMGUI_CHECKVERSION();

        ImGui::CreateContext();

        ImGui_ImplWin32_Init(m_hWnd);
        ImGui_ImplDX11_Init(GraphicsDevice::Get().GetDevice().Get(), GraphicsDevice::Get().GetDeviceContext().Get());

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        io.Fonts->Clear();

        ImFontConfig cfg{};
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;

        const ImWchar* ranges = io.Fonts->GetGlyphRangesKorean();

        io.Fonts->AddFontFromFileTTF("Resource/Font/malgun.ttf", 18.0f, &cfg, ranges);

        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();

        SoundSystem::Get().Initialize();
        
        // VFS 초기화 및 패키지 파일 마운트
        auto& vfs = VirtualFileSystem::Get();
#ifdef _DEBUG
        // 개발 모드: 실제 파일 시스템 사용
        vfs.SetDevelopmentMode(true);
#else
        // 릴리즈 모드: 패키지 파일 사용
        vfs.SetDevelopmentMode(false);
        vfs.Mount("Resource.pak");
#endif
        
        AssetManager::Get().Initialize();
        ResourceManager::Get().Initialize();

        GraphicsDevice::Get().InitializeResources();

        PreloadManager::Get().Initialize(); // preload 데이터 로드 및 global 리소스 로드

        SceneManager::Get().Initialize();

        SystemManager::Get().GetRenderSystem().Initialize();
        SystemManager::Get().GetParticleSystem().Initialize();

        // Physics 시스템 초기화
        SystemManager::Get().GetPhysicsSystem().Initialize();


#ifdef _DEBUG
        // 디버그 렌더러 초기화
        PathfindingDebugRenderer::Get().Initialize();
        PhysicsDebugRenderer::Get().Initialize();
        LightDebugRenderer::Get().Initialize();
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

        EditorManager::Get().Shutdown();       
        SceneManager::Get().Shutdown();
        AssetManager::Get().Shutdown();
        SoundSystem::Get().Shutdown();
        SystemManager::Get().Shutdown();
        ResourceManager::Get().Cleanup();
        GraphicsDevice::Get().Shutdown();

        // 디버그 렌더러 정리
        PathfindingDebugRenderer::Get().Shutdown();
        PhysicsDebugRenderer::Get().Shutdown();
        LightDebugRenderer::Get().Shutdown();
        // VFS 언마운트
        VirtualFileSystem::Get().Unmount();
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

    void WinApp::SetUserSettings(const UserSettings& userSettings)
    {
        const bool windowChanged =
            (m_userSettings.window.resolutionWidth != userSettings.window.resolutionWidth) ||
            (m_userSettings.window.resolutionHeight != userSettings.window.resolutionHeight) ||
            (m_userSettings.window.isFullscreen != userSettings.window.isFullscreen) ||
            (m_userSettings.window.useVsync != userSettings.window.useVsync);

        m_userSettings = userSettings;

        if (windowChanged)
        {
            const auto& ws = m_userSettings.window;
            SetResolution(ws.resolutionWidth, ws.resolutionHeight, ws.isFullscreen);
        }

        // 2) 사운드 설정 적용
        const auto& audio = m_userSettings.audio;
        auto& sound = SoundSystem::Get();

        sound.SetMasterVolume(audio.master);  //(미구현, 기본값 1.0f)
        sound.SetBGMVolume(audio.bgm);
        sound.SetSFXVolume(audio.sfx);
        sound.SetMute(audio.mute);

        // 3) 컨트롤 설정 적용
        //const auto& ctrl = m_userSettings.controls;
        //Input::SetMouseSensitivity(ctrl.mouseSensitivity);
        //Input::SetInvertY(ctrl.invertY);
    }

    void WinApp::ApplyResolution(int width, int height, bool isFullscreen)
    {
        auto& ws = m_userSettings.window;

        // 1) 설정 값 갱신
        if (width > 0)  ws.resolutionWidth = width;
        if (height > 0) ws.resolutionHeight = height;
        
        if (ws.resolutionWidth == width &&
            ws.resolutionHeight == height &&
            ws.isFullscreen == isFullscreen)
        {
            return;
        }

        ws.resolutionWidth = width;
        ws.resolutionHeight = height;
        ws.isFullscreen = isFullscreen;

        // 2) 실제 시스템 반영
        SetResolution(
            ws.resolutionWidth,
            ws.resolutionHeight,
            ws.isFullscreen
        );

        SaveUserSettings();
    }

    void WinApp::SaveUserSettings()
    {
        UserSettingsLoader::Save(m_settingFilePath, m_userSettings);
    }

    void WinApp::Update()
    {
        Profiling::UpdateFPS(false);
        Time::Update();
        Input::Update();

#ifdef _DEBUG
        switch (EditorManager::Get().GetEditorState())
        {
        case EditorState::Edit:
            SceneManager::Get().CheckSceneChanged(false);
            if (SceneManager::Get().GetSceneState() == SceneState::Loading)
            {
                SceneManager::Get().ProcessResourceLoading();

                if (SceneManager::Get().GetSceneState() == SceneState::Loading)
                {
                    return;
                }
            }
            SceneManager::Get().ProcessPendingAdds(false);

            EditorManager::Get().Update();
            break;

        case EditorState::Play:
            GamePlayUpdate();
            break;

        case EditorState::Pause:
            EditorManager::Get().Update();
            break;
        }
#else
        GamePlayUpdate();
#endif // _DEBUG

        SoundSystem::Get().Update();
        Profiling::UpdateMemoryUsage();
    }

    void WinApp::Render()
    {
        if (SceneManager::Get().GetSceneState() == SceneState::Loading)
        {
            GraphicsDevice::Get().ClearAllViews();
            SceneManager::Get().RenderLoadingScreen();
#ifdef _DEBUG
            EditorManager::Get().Render();
#endif //_DEBUG
            GraphicsDevice::Get().EndDraw();

            return;
        }

        SystemManager::Get().GetRenderSystem().Render();

#ifdef _DEBUG
        EditorManager::Get().Render();
#endif //_DEBUG

        GraphicsDevice::Get().EndDraw();

        SystemManager::Get().GetTransformSystem().UnmarkDirtyThisFrame();
    }

    void WinApp::GamePlayUpdate()
    {
        SceneManager::Get().CheckSceneChanged(true);

        if (SceneManager::Get().GetSceneState() == SceneState::Loading)
        {
            SceneManager::Get().ProcessResourceLoading();

            if (SceneManager::Get().GetSceneState() == SceneState::Loading)
            {
                return;
            }
        }

        SceneManager::Get().ProcessPendingAdds(true);
        SceneManager::Get().ProcessPendingKills();

        SceneManager::Get().CallOnSceneStart();

        SystemManager::Get().GetScriptSystem().CallStart();
        SystemManager::Get().GetScriptSystem().CallUpdate();

        SystemManager::Get().GetPathfindingSystem().Update();
        SystemManager::Get().GetParticleSystem().Update();

        // Physics 시뮬레이션 (내부에서 Fixed Timestep Accumulator 관리)
        // - PhysicsSystem이 자체적으로 fixedDeltaTime 단위로 시뮬레이션
        // - ScriptSystem.CallFixedUpdate()도 PhysicsSystem 내부에서 호출됨
        // - CollisionSystem.ProcessEvents()도 PhysicsSystem 내부에서 호출됨
        SystemManager::Get().GetPhysicsSystem().Update(Time::DeltaTime());

        SystemManager::Get().GetCameraSystem().Update();

        SystemManager::Get().GetAnimatorSystem().Update();

        SystemManager::Get().GetRenderSystem().Update();

        SystemManager::Get().GetUIEventSystem().Update();

        SystemManager::Get().GetScriptSystem().CallLateUpdate();
    }

    LRESULT WinApp::MessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        // ImGui 컨텍스트가 있을 때만 처리
        if (ImGui::GetCurrentContext() != nullptr)
        {
            // 우리가 만든 함수로 가로채기
            if (HandleImGuiInput(hWnd, uMsg, wParam, lParam))
            {
                return true;
            }
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
        UserSettingsLoader::Load(m_settingFilePath, m_userSettings);

        auto& ws = m_userSettings.window;

        Resolution currentRes{ ws.resolutionWidth, ws.resolutionHeight };
        Resolution screenRes{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

        bool isValid = false;
        for (const auto& supported : g_supportedResolutions)
        {
            if (supported == currentRes)
            {
                if (ws.isFullscreen)
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

                if (ws.isFullscreen)
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

        ws.resolutionWidth = currentRes.width;
        ws.resolutionHeight = currentRes.height;

        ws.supportedResolutions.clear();
        for (const auto& res : g_supportedResolutions)
        {
            ws.supportedResolutions.push_back(std::format("{}x{}", res.width, res.height));
        }

        UserSettingsLoader::Save(m_settingFilePath, m_userSettings);
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
        auto& ws = m_userSettings.window;

        if (width == 0)
        {
            width = ws.resolutionWidth;
        }
        if (height == 0)
        {
            height = ws.resolutionHeight;
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

        ws.resolutionWidth = width;
        ws.resolutionHeight = height;
        ws.isFullscreen = isFullscreen;

        SetWindowPos(m_hWnd, HWND_TOP, x, y, actualW, actualH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        GraphicsDevice::Get().Resize(
            ws.resolutionWidth,
            ws.resolutionHeight,
            m_screenWidth,
            m_screenHeight,
            isFullscreen
        );

        UpdateViewportTransformData();
    }

    void WinApp::UpdateViewportTransformData()
    {
        auto& ws = m_userSettings.window;

        float targetAspectRatio = static_cast<float>(ws.resolutionWidth) / static_cast<float>(ws.resolutionHeight);
        float screenAspectRatio = static_cast<float>(m_screenWidth) / static_cast<float>(m_screenHeight);

        float viewWidth = static_cast<float>(m_screenWidth);
        float viewHeight = static_cast<float>(m_screenHeight);
        float viewX = 0.0f;
        float viewY = 0.0f;

        if (ws.isFullscreen)
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
            // 창 모드일 때는 창 크기 = 해상도 크기라고 가정 (필러박스 없음)
            // 만약 창 모드에서도 레터박스를 쓴다면 로직 수정 필요
            viewWidth = static_cast<float>(ws.resolutionWidth);
            viewHeight = static_cast<float>(ws.resolutionHeight);
        }

        // 멤버 변수에 저장
        m_viewportData.viewX = viewX;
        m_viewportData.viewY = viewY;
        m_viewportData.width = viewWidth;
        m_viewportData.height = viewHeight;

        // 모니터 뷰포트 크기 -> 게임 해상도 크기로 가는 비율
        m_viewportData.scaleX = static_cast<float>(ws.resolutionWidth) / viewWidth;
        m_viewportData.scaleY = static_cast<float>(ws.resolutionHeight) / viewHeight;

        // 기존 Input 시스템에도 적용
        Input::SetCoordinateTransform(viewX, viewY, m_viewportData.scaleX, m_viewportData.scaleY);
    }

    bool WinApp::HandleImGuiInput(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        // 마우스 좌표가 포함된 메시지인지 확인 (휠 제외)
        bool isMouseCoordMsg = false;
        switch (uMsg)
        {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
            isMouseCoordMsg = true;
            break;
        }

        // 좌표 변환이 필요한 경우
        if (isMouseCoordMsg)
        {
            // LPARAM에서 OS 기준 좌표 추출
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // 좌표 변환 (Window -> Game Resolution)
            float localX = (x - m_viewportData.viewX) * m_viewportData.scaleX;
            float localY = (y - m_viewportData.viewY) * m_viewportData.scaleY;

            LPARAM newLParam = MAKELPARAM(static_cast<short>(localX), static_cast<short>(localY));

            return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, newLParam);
        }

        // 마우스 좌표와 상관없는 메시지 (키보드, 휠 등)는 그대로 전달
        return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    }
}
