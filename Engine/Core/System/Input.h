#pragma once

#include <directXTK/Mouse.h>
#include <directXTK/Keyboard.h>

namespace engine
{
	class Input
	{
	public:
		enum class Button
		{
			LEFT,
			RIGHT,
			MIDDLE,
			SIDE_FRONT,
			SIDE_BACK,
			MAX
		};

		static void Initialize(HWND hWnd);
		static void Update();

		static bool IsKeyHeld(DirectX::Keyboard::Keys key);
		static bool IsKeyPressed(DirectX::Keyboard::Keys key);
		static bool IsKeyReleased(DirectX::Keyboard::Keys key);

		static bool IsMouseHeld(Input::Button button);
		static bool IsMousePressed(Input::Button button);
		static bool IsMouseReleased(Input::Button button);

		static void SetMouseMode(DirectX::Mouse::Mode mode);
		static Vector2 GetMouseDelta();
		static Vector2 GetMousePosition();
	};
}