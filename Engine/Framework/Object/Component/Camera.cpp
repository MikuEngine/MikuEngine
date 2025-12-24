#include "pch.h"
#include "Camera.h"

#include "Framework/Object/Component/Transform.h"
#include "Common/Math/MathUtility.h"

namespace engine
{
    void Camera::Update()
    {
        auto transform = GetTransform();

        if (bool isDirtyThisFrame = transform->IsDirtyThisFrame(); m_isDirty || isDirtyThisFrame)
        {
            if (isDirtyThisFrame)
            {
                Matrix world = transform->GetWorld();

                Vector3 scale;
                Quaternion rotation;
                Vector3 translation;
                world.Decompose(scale, rotation, translation);

                const Vector3 forward = Vector3::Transform(Vector3::Forward, rotation);
                const Vector3 up = Vector3::Transform(Vector3::Up, rotation);

                m_world = Matrix::CreateWorld(translation, forward, up);
            }

            // view
            {
                m_view = DirectX::XMMatrixLookAtLH(
                    m_world.Translation(),
                    m_world.Translation() - m_world.Forward(),
                    m_world.Up());
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

            m_isDirty = false;
        }
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