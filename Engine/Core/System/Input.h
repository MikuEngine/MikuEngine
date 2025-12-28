#pragma once

namespace engine
{
    class Input
    {
    public:
        enum class Buttons
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

        static bool IsMouseHeld(Input::Buttons button);
        static bool IsMousePressed(Input::Buttons button);
        static bool IsMouseReleased(Input::Buttons button);

        static void SetMouseMode(DirectX::Mouse::Mode mode);
        static void SetCoordinateTransform(float offsetX, float offsetY, float scaleX, float scaleY);

        static Vector2 GetMouseDelta();
        static Vector2 GetMousePosition();
    };
}
