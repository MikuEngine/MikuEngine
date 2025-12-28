#include "pch.h"
#include "EditorCameraController.h"

#include "Framework/Object/Component/Transform.h"

namespace game
{
    void EditorCameraController::Update()
    {
		using namespace DirectX;
		using Keys = DirectX::Keyboard::Keys;

		const engine::Vector3 forward = GetTransform()->GetForward();
		const engine::Vector3 right = GetTransform()->GetRight();
		const engine::Vector3 up = GetTransform()->GetUp();
		engine::Vector3 position = GetTransform()->GetLocalPosition();
		engine::Vector3 rotation = GetTransform()->GetLocalEulerAngles();

		engine::Vector3 inputVector{ 0.0f, 0.0f, 0.0f };

		if (engine::Input::IsKeyHeld(Keys::W))
		{
			inputVector += forward;
		}
		else if (engine::Input::IsKeyHeld(Keys::S))
		{
			inputVector -= forward;
		}

		if (engine::Input::IsKeyHeld(Keys::D))
		{
			inputVector += right;
		}
		else if (engine::Input::IsKeyHeld(Keys::A))
		{
			inputVector -= right;
		}

		if (engine::Input::IsKeyHeld(Keys::E))
		{
			inputVector += up;
		}
		else if (engine::Input::IsKeyHeld(Keys::Q))
		{
			inputVector -= up;
		}

		inputVector.Normalize();

		float speed = 50.0f;

		if (engine::Input::IsKeyHeld(Keys::LeftShift))
		{
			speed = speed * 2;
		}

		position += inputVector * speed * engine::Time::DeltaTime();

		if (engine::Input::IsMouseHeld(engine::Input::Buttons::RIGHT))
		{
			engine::Input::SetMouseMode(Mouse::Mode::MODE_RELATIVE);

			engine::Vector2 mouseDelta = engine::Input::GetMouseDelta();

			rotation.x += XMConvertToRadians(mouseDelta.y * 5.0f);
			rotation.y += XMConvertToRadians(mouseDelta.x * 5.0f);
		}
		else
		{
			engine::Input::SetMouseMode(Mouse::Mode::MODE_ABSOLUTE);
		}

		GetTransform()->SetLocalPosition(position);
		GetTransform()->SetLocalRotation(rotation);
    }
	
	std::string EditorCameraController::GetType() const
	{
		return "EditorCameraController";
	}
}