#include "pch.h"
#include "WinApp.h"

#include <DirectXColors.h>

#include "ConfigLoader.h"
#include "Common/Utility/Profiling.h"

namespace engine
{
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	WinApp::WinApp(const std::string& settingFilePath, const WindowSettings& defaultSetting)
		: m_settingFilePath{ settingFilePath }, m_settings{ defaultSetting }
	{
		ConfigLoader::Load(settingFilePath, m_settings);
	}

	WinApp::~WinApp()
	{
		Shutdown();
	}

	void WinApp::Initialize()
	{
		if (m_settings.isFullScreen)
		{
			m_windowStyle = WS_POPUP | WS_VISIBLE;
		}
		else
		{
			m_windowStyle = WS_OVERLAPPEDWINDOW;
			if (!m_settings.isResizable)
			{
				m_windowStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
			}
		}

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

		RECT rc{ 0, 0, m_settings.width, m_settings.height };
		AdjustWindowRect(&rc, m_windowStyle, FALSE);

		int actualWidth = rc.right - rc.left;
		int actualHeight = rc.bottom - rc.top;

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		bool settingChanged = false;

		if (m_settings.width > screenWidth)
		{
			m_settings.width = screenWidth;
			settingChanged = true;
		}

		if (m_settings.height > screenHeight)
		{
			m_settings.height = screenHeight;
			settingChanged = true;
		}

		if (settingChanged)
		{
			ConfigLoader::Save(m_settingFilePath, m_settings);
		}

		m_x = (screenWidth - actualWidth) / 2 < 0 ? 0 : (screenWidth - actualWidth) / 2;
		m_y = (screenHeight - actualHeight) / 2 < 0 ? 0 : (screenHeight - actualHeight) / 2;

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

		m_graphicsDevice.Initialize(
			m_hWnd,
			static_cast<UINT>(m_settings.width),
			static_cast<UINT>(m_settings.height),
			m_settings.useVsync);

		Input::Initialize(m_hWnd);
	}

	void WinApp::Shutdown()
	{
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
	}

	void WinApp::Render()
	{
		// final
		m_graphicsDevice.BeginDraw();
		m_graphicsDevice.EndDraw();
	}

	LRESULT WinApp::MessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
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
}
