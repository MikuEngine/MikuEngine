#include "EnginePCH.h"
#include "CapsuleCollider.h"

#include "Framework/Physics/PhysicsUtility.h"

namespace engine
{
    void CapsuleCollider::SetRadius(float radius)
    {
        m_radius = std::max(0.001f, radius);
        
        // 높이가 반지름×2보다 작으면 조정
        if (m_height < m_radius * 2.0f)
        {
            m_height = m_radius * 2.0f;
        }
        
        UpdateGeometry();
    }

    void CapsuleCollider::SetHeight(float height)
    {
        // 최소 높이는 반지름×2 (양쪽 반구)
        m_height = std::max(m_radius * 2.0f, height);
        UpdateGeometry();
    }

    physx::PxGeometry* CapsuleCollider::CreateGeometry()
    {
        // PhysX 캡슐: 기본적으로 X축 방향
        // halfHeight = 실린더 부분의 절반 높이
        return new physx::PxCapsuleGeometry(m_radius, GetHalfHeight());
    }

    void CapsuleCollider::UpdateGeometry()
    {
        if (!m_shape)
        {
            return;
        }

        physx::PxCapsuleGeometry capsule(m_radius, GetHalfHeight());
        m_shape->setGeometry(capsule);

        UpdateLocalPose();
    }

    void CapsuleCollider::UpdateLocalPose()
    {
        if (!m_shape)
        {
            return;
        }

        // 사용자 회전 (오일러 각도 -> 쿼터니언)
        Vector3 radians = m_rotation * (DirectX::XM_PI / 180.0f);
        Quaternion userRot = Quaternion::CreateFromYawPitchRoll(radians.y, radians.x, radians.z);

        // PhysX 캡슐은 X축 방향이 기본 -> Y축 방향으로 변환 (Z축 90도 회전)
        Quaternion baseRot = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, DirectX::XM_PIDIV2);

        // 기본 회전 먼저, 그 다음 사용자 회전 적용
        Quaternion finalRot = baseRot * userRot;

        physx::PxTransform localPose(
            PhysicsUtility::ToPxVec3(m_center),
            physx::PxQuat(finalRot.x, finalRot.y, finalRot.z, finalRot.w)
        );
        m_shape->setLocalPose(localPose);
    }

    void CapsuleCollider::OnGui()
    {
        Collider::OnGui();
        
        ImGui::Separator();
        
        // Radius
        float radius = m_radius;
        if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.001f, 1000.0f))
        {
            SetRadius(radius);
        }
        
        // Height
        float height = m_height;
        if (ImGui::DragFloat("Height", &height, 0.01f, m_radius * 2.0f, 1000.0f))
        {
            SetHeight(height);
        }
    }

    void CapsuleCollider::Save(json& j) const
    {
        Collider::Save(j);
        j["radius"] = m_radius;
        j["height"] = m_height;
    }

    void CapsuleCollider::Load(const json& j)
    {
        Collider::Load(j);
        if (j.contains("radius"))
        {
            m_radius = j["radius"].get<float>();
        }
        if (j.contains("height"))
        {
            m_height = j["height"].get<float>();
        }
    }
}
