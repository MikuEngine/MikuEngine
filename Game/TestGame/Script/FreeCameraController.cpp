#include "GamePCH.h"
#include "FreeCameraController.h"

namespace game
{
    void FreeCameraController::Update()
    {
        const float deltaTime = engine::Time::DeltaTime();

        if (engine::Input::IsMouseHeld(engine::Buttons::RIGHT))
        {
            engine::Input::SetMouseMode(DirectX::Mouse::Mode::MODE_RELATIVE);

            const engine::Vector2 mouseDelta = engine::Input::GetMouseDelta();

            auto angles = GetTransform()->GetLocalEulerAngles();

            angles.y += mouseDelta.x * m_rotationSpeed;
            angles.x += mouseDelta.y * m_rotationSpeed;

            constexpr float limit = 89.9f;
            angles.x = std::clamp(angles.x, -limit, limit);

            GetTransform()->SetLocalRotation(angles);
        }
        else
        {
            engine::Input::SetMouseMode(DirectX::Mouse::Mode::MODE_ABSOLUTE);
        }


        const engine::Vector3 forward = GetTransform()->GetForward();
        const engine::Vector3 right = GetTransform()->GetRight();

        engine::Vector3 moveDir = engine::Vector3::Zero;

        if (engine::Input::IsKeyHeld(engine::Keys::W))
        {
            moveDir += forward;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::S))
        {
            moveDir -= forward;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::D))
        {
            moveDir += right;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::A))
        {
            moveDir -= right;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::E))
        {
            moveDir += engine::Vector3::UnitY;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::Q))
        {
            moveDir -= engine::Vector3::UnitY;
        }

        if (moveDir != engine::Vector3::Zero)
        {
            moveDir.Normalize();

            float speed = m_moveSpeed;
            if (engine::Input::IsKeyHeld(engine::Keys::LeftShift))
            {
                speed *= 2.0f;
            }

            auto position = GetTransform()->GetLocalPosition();

            position += moveDir * speed * deltaTime;

            GetTransform()->SetLocalPosition(position);
        }
    }

    void FreeCameraController::OnGui()
    {
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, FLT_MAX, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    }

    void FreeCameraController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["MoveSpeed"] = m_moveSpeed;
    }

    void FreeCameraController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "MoveSpeed", m_moveSpeed);
    }

    std::string FreeCameraController::GetType() const
    {
        return "FreeCameraController";
    }
}