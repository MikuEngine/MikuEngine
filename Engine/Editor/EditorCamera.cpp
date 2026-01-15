#include "EnginePCH.h"
#include "EditorCamera.h"

namespace engine
{
    void EditorCamera::Update()
    {
        const float deltaTime = Time::DeltaTime();

        if (Input::IsMouseHeld(Buttons::RIGHT))
        {
            Input::SetMouseMode(DirectX::Mouse::Mode::MODE_RELATIVE);

            const Vector2 mouseDelta = Input::GetMouseDelta();

            m_yaw += mouseDelta.x * m_rotationSpeed;
            m_pitch += mouseDelta.y * m_rotationSpeed;

            constexpr float limit = ToRadian(89.9f);
            m_pitch = std::clamp(m_pitch, -limit, limit);
            m_isDirty = true;
        }
        else
        {
            engine::Input::SetMouseMode(DirectX::Mouse::Mode::MODE_ABSOLUTE);
        }

        const Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);

        const Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotationMatrix);
        const Vector3 right = Vector3::Transform(Vector3::UnitX, rotationMatrix);
        const Vector3 up = Vector3::Transform(Vector3::UnitY, rotationMatrix);

        Vector3 moveDir = Vector3::Zero;

        if (Input::IsKeyHeld(Keys::W))
        {
            moveDir += forward;
        }

        if (Input::IsKeyHeld(Keys::S))
        {
            moveDir -= forward;
        }

        if (Input::IsKeyHeld(Keys::D))
        {
            moveDir += right;
        }

        if (Input::IsKeyHeld(Keys::A))
        {
            moveDir -= right;
        }

        if (Input::IsKeyHeld(Keys::E))
        {
            moveDir += Vector3::UnitY;
        }

        if (Input::IsKeyHeld(Keys::Q))
        {
            moveDir -= Vector3::UnitY;
        }

        if (moveDir != Vector3::Zero)
        {
            moveDir.Normalize();

            float speed = m_moveSpeed;
            if (Input::IsKeyHeld(Keys::LeftShift))
            {
                speed *= 2.0f;
            }

            m_position += moveDir * speed * deltaTime;
            m_isDirty = true;
        }

        if (m_isDirty)
        {
            m_view = XMMatrixLookToLH(m_position, forward, Vector3::UnitY);

            m_projection = DirectX::XMMatrixPerspectiveFovLH(ToRadian(m_fov), m_aspectRatio, m_near, m_far);

            m_isDirty = false;
        }
    }

    void EditorCamera::OnGui()
    {
        ImGui::PushID(this);
        if (ImGui::DragFloat3("Position", &m_position.x))
        {
            m_isDirty = true;
        }
        ImGui::DragFloat("MoveSpeed", &m_moveSpeed, 0.1f, 0.0f, 2000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

        if (float nearFar[]{ m_near, m_far }; ImGui::DragFloat2("Near/Far", nearFar, 0.1f))
        {
            if (nearFar[0] > 10.0f)
            {
                nearFar[0] = 10.0f;
            }

            if (nearFar[0] < 0.01f)
            {
                nearFar[0] = 0.01f;
            }

            if (nearFar[1] < 100.0f)
            {
                nearFar[1] = 100.0f;
            }

            m_near = nearFar[0];
            m_far = nearFar[1];

            m_isDirty = true;
        }

        ImGui::PopID();
    }

    const Matrix& EditorCamera::GetView() const
    {
        return m_view;
    }

    const Matrix& EditorCamera::GetProjection() const
    {
        return m_projection;
    }

    const Vector3& EditorCamera::GetPosition() const
    {
        return m_position;
    }

    float EditorCamera::GetFar() const
    {
        return m_far;
    }

    void EditorCamera::SetPosition(const Vector3& position)
    {
        m_position = position;

        m_isDirty = true;
    }

    void EditorCamera::SetCameraInfo(float fov, float aspect, float nearZ, float farZ)
    {
        m_fov = fov;
        m_aspectRatio = aspect;
        m_near = nearZ;
        m_far = farZ;

        m_isDirty = true;
    }
}