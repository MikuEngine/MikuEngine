#include "EnginePCH.h"
#include "SphereCollider.h"

namespace engine
{
    void SphereCollider::SetRadius(float radius)
    {
        m_radius = std::max(0.001f, radius);
        UpdateGeometry();
    }

    physx::PxGeometry* SphereCollider::CreateGeometry()
    {
        return new physx::PxSphereGeometry(m_radius);
    }

    void SphereCollider::UpdateGeometry()
    {
        if (!m_shape)
        {
            return;
        }

        physx::PxSphereGeometry sphere(m_radius);
        m_shape->setGeometry(sphere);
    }

    void SphereCollider::OnGui()
    {
        Collider::OnGui();
        
        ImGui::Separator();
        
        // Radius
        float radius = m_radius;
        if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.001f, 1000.0f))
        {
            SetRadius(radius);
        }
    }

    void SphereCollider::Save(json& j) const
    {
        Collider::Save(j);
        j["radius"] = m_radius;
    }

    void SphereCollider::Load(const json& j)
    {
        Collider::Load(j);
        if (j.contains("radius"))
        {
            m_radius = j["radius"].get<float>();
        }
    }
}
