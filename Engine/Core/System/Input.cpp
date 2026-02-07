#include "EnginePCH.h"
#include "Input.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    namespace
    {
        using ButtonState = DirectX::Mouse::ButtonStateTracker::ButtonState;

        DirectX::Keyboard g_keyboard;
        DirectX::Keyboard::State g_keyboardState;
        DirectX::Keyboard::KeyboardStateTracker g_keyboardStateTracker;

        DirectX::Mouse g_mouse;
        DirectX::Mouse::State g_mouseState;
        DirectX::Mouse::ButtonStateTracker g_mouseStateTracker;

        bool* g_mouseHeldStateTable[static_cast<size_t>(Input::Buttons::MAX)];
        ButtonState* g_mouseStateTable[static_cast<size_t>(Input::Buttons::MAX)];

        float g_offsetX = 0.0f;
        float g_offsetY = 0.0f;
        float g_scaleX = 1.0f;
        float g_scaleY = 1.0f;

        int g_prevWheel = 0;
        float g_wheelDelta = 0.0f;

        Vector2 g_virtualMousePos = { 0.0f, 0.0f };
        float g_mouseSensitivity = 1.0f;

        HWND g_hWnd = nullptr;

        bool g_isLockMode = false;
        bool g_isFirstFrameAfterLock = false;
    }

    void Input::Initialize(HWND hWnd)
    {
        g_mouse.SetWindow(hWnd);

        g_mouseHeldStateTable[static_cast<size_t>(Input::Buttons::LEFT)] = &g_mouseState.leftButton;
        g_mouseHeldStateTable[static_cast<size_t>(Input::Buttons::RIGHT)] = &g_mouseState.rightButton;
        g_mouseHeldStateTable[static_cast<size_t>(Input::Buttons::MIDDLE)] = &g_mouseState.middleButton;
        g_mouseHeldStateTable[static_cast<size_t>(Input::Buttons::SIDE_FRONT)] = &g_mouseState.xButton1;
        g_mouseHeldStateTable[static_cast<size_t>(Input::Buttons::SIDE_BACK)] = &g_mouseState.xButton2;

        g_mouseStateTable[static_cast<size_t>(Input::Buttons::LEFT)] = &g_mouseStateTracker.leftButton;
        g_mouseStateTable[static_cast<size_t>(Input::Buttons::RIGHT)] = &g_mouseStateTracker.rightButton;
        g_mouseStateTable[static_cast<size_t>(Input::Buttons::MIDDLE)] = &g_mouseStateTracker.middleButton;
        g_mouseStateTable[static_cast<size_t>(Input::Buttons::SIDE_FRONT)] = &g_mouseStateTracker.xButton1;
        g_mouseStateTable[static_cast<size_t>(Input::Buttons::SIDE_BACK)] = &g_mouseStateTracker.xButton2;

        g_mouseState = g_mouse.GetState();
        g_prevWheel = g_mouseState.scrollWheelValue;
        g_wheelDelta = 0.0f;

        g_virtualMousePos.x = static_cast<float>(g_mouseState.x);
        g_virtualMousePos.y = static_cast<float>(g_mouseState.y);

        g_prevWheel = g_mouseState.scrollWheelValue;

        g_hWnd = hWnd;
    }

    void Input::Update()
    {
        g_keyboardState = g_keyboard.GetState();
        g_keyboardStateTracker.Update(g_keyboardState);

        g_mouseState = g_mouse.GetState();
        g_mouseStateTracker.Update(g_mouseState);

        if (g_isLockMode)
        {
            if (g_isFirstFrameAfterLock)
            {
                g_isFirstFrameAfterLock = false;
                // 현재 휠 값만 동기화하고 좌표 계산은 스킵
                g_prevWheel = g_mouseState.scrollWheelValue;
                return;
            }

            // Relative 모드에서는 x, y 자체가 델타값(이동량)입니다.
            float dx = static_cast<float>(g_mouseState.x);
            float dy = static_cast<float>(g_mouseState.y);

            if (dx != 0 || dy != 0)
            {
                // 감도 적용하여 가상 좌표 업데이트
                g_virtualMousePos.x += dx * g_mouseSensitivity;
                g_virtualMousePos.y += dy * g_mouseSensitivity;
            }
        }
        else
        {
            // 2. Unlock(에디터/메뉴) 모드일 때 (Absolute 좌표 사용)
            g_virtualMousePos.x = static_cast<float>(g_mouseState.x);
            g_virtualMousePos.y = static_cast<float>(g_mouseState.y);
        }

        // 5. 클램프
        auto vp = GraphicsDevice::Get().GetViewport();
        g_virtualMousePos.x = std::clamp(g_virtualMousePos.x, 0.0f, vp.Width);
        g_virtualMousePos.y = std::clamp(g_virtualMousePos.y, 0.0f, vp.Height);

        const int nowWheel = g_mouseState.scrollWheelValue;
        g_wheelDelta = static_cast<float>(nowWheel - g_prevWheel);
        g_prevWheel = nowWheel;
    }

    void Input::SetMouseSensitivity(float v)
    {
        g_mouseSensitivity = std::max(0.001f, v);
    }

    bool Input::IsKeyHeld(DirectX::Keyboard::Keys key)
    {
        return g_keyboardState.IsKeyDown(key);
    }

    bool Input::IsKeyPressed(DirectX::Keyboard::Keys key)
    {
        return g_keyboardStateTracker.IsKeyPressed(key);
    }

    bool Input::IsKeyReleased(DirectX::Keyboard::Keys key)
    {
        return g_keyboardStateTracker.IsKeyReleased(key);
    }

    bool Input::IsMouseHeld(Input::Buttons button)
    {
        return *g_mouseHeldStateTable[static_cast<size_t>(button)];
    }

    bool Input::IsMousePressed(Input::Buttons button)
    {
        return *g_mouseStateTable[static_cast<size_t>(button)] == ButtonState::PRESSED;
    }

    bool Input::IsMouseReleased(Input::Buttons button)
    {
        return *g_mouseStateTable[static_cast<size_t>(button)] == ButtonState::RELEASED;
    }

    void Input::SetMouseMode(DirectX::Mouse::Mode mode)
    {
        g_mouse.SetMode(mode);
    }

    void Input::SetCoordinateTransform(float offsetX, float offsetY, float scaleX, float scaleY)
    {
        g_offsetX = offsetX;
        g_offsetY = offsetY;
        g_scaleX = scaleX;
        g_scaleY = scaleY;
    }

    Vector2 Input::GetMouseDelta()
    {
        if (g_mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
        {
            return { static_cast<float>(g_mouseState.x), static_cast<float>(g_mouseState.y) };
        }

        return { 0.0f, 0.0f };
    }

    Vector2 Input::GetMousePosition()
    {
        float x = (g_virtualMousePos.x - g_offsetX) / g_scaleX;
        float y = (g_virtualMousePos.y - g_offsetY) / g_scaleY;

        return { x, y };
    }

    float Input::GetMouseWheelDelta()
    {
        return g_wheelDelta;
    }

    float Input::GetMouseWheelNotch()
    {
        constexpr float WHEEL_TICK = 120.0f;
        return (WHEEL_TICK > 0.0f) ? (g_wheelDelta / WHEEL_TICK) : 0.0f;
    }

    void Input::ConfineCursor(HWND hWnd, bool confine)
    {
        if (confine)
        {
            RECT rect;
            // 현재 윈도우의 클라이언트 영역(실제 그림이 그려지는 영역) 좌표를 가져옴
            GetClientRect(hWnd, &rect);

            // 클라이언트 좌표를 스크린 좌표(모니터 전체 기준)로 변환
            POINT tl = { rect.left, rect.top };
            POINT br = { rect.right, rect.bottom };
            ClientToScreen(hWnd, &tl);
            ClientToScreen(hWnd, &br);

            RECT screenRect = { tl.x, tl.y, br.x, br.y };

            // 마우스 커서를 이 사각형 안에 가둠
            ::ClipCursor(&screenRect);
        }
        else
        {
            ::ClipCursor(nullptr);
        }
    }

    void Input::SetLockMode(bool lock)
    {
        if (g_isLockMode == lock) return;

        if (lock)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(g_hWnd, &pt);

            g_virtualMousePos.x = static_cast<float>(pt.x);
            g_virtualMousePos.y = static_cast<float>(pt.y);

            // 윈도우가 다른 곳을 보고 있을 수 있으니 포커스를 다시 가져옴
            SetFocus(g_hWnd);

            // 모드 변경 (이 순간 마우스는 중앙으로 숨음)
            g_mouse.SetMode(DirectX::Mouse::MODE_RELATIVE);

            // 다음 Update에서 델타값을 무시하도록 설정
            g_isFirstFrameAfterLock = true;
        }
        else
        {
            g_mouse.SetMode(DirectX::Mouse::MODE_ABSOLUTE);

            POINT pt = { (int)g_virtualMousePos.x, (int)g_virtualMousePos.y };
            ClientToScreen(g_hWnd, &pt);
            SetCursorPos(pt.x, pt.y);
        }

        g_isLockMode = lock;
    }

    void Input::SyncVirtualMouseFromOS()
    {
        SetFocus(g_hWnd);

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(g_hWnd, &pt);

        g_virtualMousePos.x = (float)pt.x;
        g_virtualMousePos.y = (float)pt.y;

        // 휠도 첫 프레임 튐 방지용 동기화
        g_mouseState = g_mouse.GetState();
        g_prevWheel = g_mouseState.scrollWheelValue;
        g_wheelDelta = 0.0f;
    }
}
