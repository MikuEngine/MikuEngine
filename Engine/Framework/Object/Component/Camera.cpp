#include "EnginePCH.h"
#include "Camera.h"

#include "Common/Math/MathUtility.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Core/Graphics/Device/GraphicsDevice.h"

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
        Object::Save(j);

        j["Near"] = m_near;
        j["Far"] = m_far;
        j["FOV"] = m_fov;
    }

    void Camera::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j, "Near", m_near);
        JsonGet(j, "Far", m_far);
        JsonGet(j, "FOV", m_fov);
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

    Vector3 Camera::GetForward() const
    {
        return engine::GetForward(m_world);
    }

    Vector3 Camera::GetPosition() const
    {
        return engine::GetTranslation(m_world);
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

    bool Camera::ScreenToWorldRay(const Vector2& screenPos, Vector3& outOrigin, Vector3& outDirection) const
    {
        // 뷰포트 정보 가져오기
        const auto& vp = GraphicsDevice::Get().GetViewport();
        const float vpW = vp.Width;
        const float vpH = vp.Height;

        if (vpW <= 0.0f || vpH <= 0.0f) return false;

        // 스크린 좌표 → NDC (Normalized Device Coordinates)
        // NDC: x, y는 [-1, 1] 범위
        const float ndcX = (screenPos.x / vpW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (screenPos.y / vpH) * 2.0f;  // Y축 반전 (스크린 좌표는 위가 0)

        // View-Projection 역행렬 계산
        Matrix viewProj = m_view * m_projection;
        Matrix invViewProj = viewProj.Invert();

        // Near plane과 Far plane에서의 월드 좌표 계산
        // NDC z=0은 near plane, z=1은 far plane (DirectX 기준)
        Vector4 nearPoint(ndcX, ndcY, 0.0f, 1.0f);
        Vector4 farPoint(ndcX, ndcY, 1.0f, 1.0f);

        // NDC → 월드 좌표
        Vector4 nearWorld = Vector4::Transform(nearPoint, invViewProj);
        Vector4 farWorld = Vector4::Transform(farPoint, invViewProj);

        // Perspective divide
        if (std::abs(nearWorld.w) < 0.0001f || std::abs(farWorld.w) < 0.0001f)
        {
            return false;
        }

        nearWorld /= nearWorld.w;
        farWorld /= farWorld.w;

        // Ray 시작점과 방향 계산
        outOrigin = Vector3(nearWorld.x, nearWorld.y, nearWorld.z);
        Vector3 farPos(farWorld.x, farWorld.y, farWorld.z);
        
        outDirection = farPos - outOrigin;
        outDirection.Normalize();

        return true;
    }

    bool Camera::WorldToScreen(const Vector3& worldPos, Vector2& outScreenPos) const
    {
        // 뷰포트 정보 가져오기
        const auto& vp = GraphicsDevice::Get().GetViewport();
        const float vpW = vp.Width;
        const float vpH = vp.Height;

        // 월드 → 클립 좌표
        Vector4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        Vector4 viewPos = Vector4::Transform(pos, m_view);
        Vector4 clipPos = Vector4::Transform(viewPos, m_projection);

        // 카메라 뒤에 있으면 실패
        if (clipPos.w <= 0.0001f)
        {
            return false;
        }

        // NDC (Normalized Device Coordinates)
        const float invW = 1.0f / clipPos.w;
        const float ndcX = clipPos.x * invW;
        const float ndcY = clipPos.y * invW;

        // NDC → 스크린 좌표
        outScreenPos.x = (ndcX * 0.5f + 0.5f) * vpW;
        outScreenPos.y = (-ndcY * 0.5f + 0.5f) * vpH;

        return true;
    }
}