#include "EnginePCH.h"
#include "BoxCollider.h"

#include "Framework/Physics/PhysicsUtility.h"

namespace engine
{
    void BoxCollider::SetSize(const Vector3& size)
    {
        m_size = Vector3(
            std::max(0.001f, size.x),
            std::max(0.001f, size.y),
            std::max(0.001f, size.z)
        );
        UpdateGeometry();
    }

    void BoxCollider::SetHalfExtents(const Vector3& halfExtents)
    {
        SetSize(halfExtents * 2.0f);
    }

    physx::PxGeometry* BoxCollider::CreateGeometry()
    {
        Vector3 halfExtents = GetHalfExtents();
        return new physx::PxBoxGeometry(
            PhysicsUtility::ToPxScale(halfExtents)
        );
    }

    void BoxCollider::UpdateGeometry()
    {
        if (!m_shape)
        {
            return;
        }

        Vector3 halfExtents = GetHalfExtents();
        physx::PxBoxGeometry box(PhysicsUtility::ToPxScale(halfExtents));
        m_shape->setGeometry(box);
    }

    void BoxCollider::ApplyScaleRatio(const Vector3& scaleRatio)
    {
        // 스케일 비율만큼 크기 조정
        m_size.x *= scaleRatio.x;
        m_size.y *= scaleRatio.y;
        m_size.z *= scaleRatio.z;
    }

    void BoxCollider::OnGui()
    {
        ImGui::Indent();
        
        Collider::OnGui();
        
        ImGui::Separator();
        
        // Size
        Vector3 size = m_size;
        if (ImGui::DragFloat3("Size", &size.x, 0.01f, 0.001f, 1000.0f))
        {
            SetSize(size);
        }
        
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    void BoxCollider::Save(json& j) const
    {
        Collider::Save(j);
        j["size"] = { m_size.x, m_size.y, m_size.z };
    }

    void BoxCollider::Load(const json& j)
    {
        Collider::Load(j);
        if (j.contains("size"))
        {
            auto& s = j["size"];
            m_size = Vector3(s[0].get<float>(), s[1].get<float>(), s[2].get<float>());
        }
    }
}
