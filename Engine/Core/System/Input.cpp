#include "pch.h"
#include "Input.h"

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

		bool* g_mouseHeldStateTable[static_cast<size_t>(Input::Button::MAX)];
		ButtonState* g_mouseStateTable[static_cast<size_t>(Input::Button::MAX)];
	}

	void Input::Initialize(HWND hWnd)
	{
		g_mouse.SetWindow(hWnd);

		g_mouseHeldStateTable[static_cast<size_t>(Input::Button::LEFT)] = &g_mouseState.leftButton;
		g_mouseHeldStateTable[static_cast<size_t>(Input::Button::RIGHT)] = &g_mouseState.rightButton;
		g_mouseHeldStateTable[static_cast<size_t>(Input::Button::MIDDLE)] = &g_mouseState.middleButton;
		g_mouseHeldStateTable[static_cast<size_t>(Input::Button::SIDE_FRONT)] = &g_mouseState.xButton1;
		g_mouseHeldStateTable[static_cast<size_t>(Input::Button::SIDE_BACK)] = &g_mouseState.xButton2;

		g_mouseStateTable[static_cast<size_t>(Input::Button::LEFT)] = &g_mouseStateTracker.leftButton;
		g_mouseStateTable[static_cast<size_t>(Input::Button::RIGHT)] = &g_mouseStateTracker.rightButton;
		g_mouseStateTable[static_cast<size_t>(Input::Button::MIDDLE)] = &g_mouseStateTracker.middleButton;
		g_mouseStateTable[static_cast<size_t>(Input::Button::SIDE_FRONT)] = &g_mouseStateTracker.xButton1;
		g_mouseStateTable[static_cast<size_t>(Input::Button::SIDE_BACK)] = &g_mouseStateTracker.xButton2;
	}

	void Input::Update()
	{
		g_keyboardState = g_keyboard.GetState();
		g_keyboardStateTracker.Update(g_keyboardState);

		g_mouseState = g_mouse.GetState();
		g_mouseStateTracker.Update(g_mouseState);
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

	bool Input::IsMouseHeld(Input::Button button)
	{
		return *g_mouseHeldStateTable[static_cast<size_t>(button)];
	}

	bool Input::IsMousePressed(Input::Button button)
	{
		return *g_mouseStateTable[static_cast<size_t>(button)] == ButtonState::PRESSED;
	}

	bool Input::IsMouseReleased(Input::Button button)
	{
		return *g_mouseStateTable[static_cast<size_t>(button)] == ButtonState::RELEASED;
	}

	void Input::SetMouseMode(DirectX::Mouse::Mode mode)
	{
		g_mouse.SetMode(mode);
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
		if (g_mouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
		{
			return { static_cast<float>(g_mouseState.x), static_cast<float>(g_mouseState.y) };
		}

		return { 0.0f, 0.0f };
	}
}
