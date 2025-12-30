#include "pch.h"
#include "Camera.h"

#include "Common/Math/MathUtility.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"

namespace engine
{
    Camera::~Camera()
    {
        SystemManager::Get().GetCameraSystem().Unregister(this);
    }

    void Camera::Initialize()
    {
        SystemManager::Get().GetCameraSystem().Register(this);
    }

    void Camera::Update()
    {
        auto transform = GetTransform();

        //if (bool isDirtyThisFrame = transform->IsDirtyThisFrame(); m_isDirty || isDirtyThisFrame)
        //{
        //    if (isDirtyThisFrame)
        //    {
                Matrix world = transform->GetWorld();

                Vector3 scale;
                Quaternion rotation;
                Vector3 translation;
                world.Decompose(scale, rotation, translation);

                const Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);
                const Vector3 up = Vector3::Transform(Vector3::UnitY, rotation);

                m_world = Matrix::CreateWorld(translation, forward, up);
            //}

            // view
            {
                m_view = DirectX::XMMatrixLookToLH(translation, forward, up);
            }

            // projection
            {
                if (m_projectionType == ProjectionType::Perspective)
                {
                    m_projection = DirectX::XMMatrixPerspectiveFovLH(ToRadian(m_fov), m_width / m_height, m_near, m_far);
                }
                else
                {
                    const float width = m_width * m_scale;
                    const float height = m_height * m_scale;

                    m_projection = DirectX::XMMatrixOrthographicLH(width, height, m_near, m_far);
                }
            }

            // frustum
            {
                m_frustum = DirectX::BoundingFrustum(m_projection);
                m_frustum.Transform(m_frustum, m_world);
            }

        //    m_isDirty = false;
        //}
    }

    void Camera::OnGui()
    {
        ImGui::DragFloat("Near", &m_near, 0.1f, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Far", &m_far, 0.1f, 100.0f, 100000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("FOV", &m_fov, 0.1f, 1.0f, 179.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    }

    void Camera::Save(json& j) const
    {
        j["Type"] = "Camera";
        j["Near"] = m_near;
        j["Far"] = m_far;
        j["FOV"] = m_fov;
    }

    void Camera::Load(const json& j)
    {
        JsonGet(j, "Near", m_near);
        JsonGet(j, "Far", m_far);
        JsonGet(j, "FOV", m_fov);
    }

    std::string Camera::GetType() const
    {
        return "Camera";
    }

    void Camera::SetProjectionType(ProjectionType type)
    {
        m_projectionType = type;
    }

    const Matrix& Camera::GetWorld() const
    {
        return m_world;
    }

    const Matrix& Camera::GetView() const
    {
        return m_view;
    }

    const Matrix& Camera::GetProjection() const
    {
        return m_projection;
    }

    const DirectX::BoundingFrustum& Camera::GetFrustum() const
    {
        return m_frustum;
    }

    void Camera::SetNear(float value)
    {
        m_near = value;

        m_isDirty = true;
    }

    void Camera::SetFar(float value)
    {
        m_far = value;

        m_isDirty = true;
    }

    void Camera::SetFov(float degree)
    {
        m_fov = degree;

        m_isDirty = true;
    }

    void Camera::SetScale(float scale)
    {
        m_scale = scale;

        m_isDirty = true;
    }

    void Camera::SetWidth(float width)
    {
        m_width = width;

        m_isDirty = true;
    }

    void Camera::SetHeight(float height)
    {
        m_height = height;

        m_isDirty = true;
    }
}