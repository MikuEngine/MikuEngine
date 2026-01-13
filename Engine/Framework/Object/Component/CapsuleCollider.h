#pragma once

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // CapsuleCollider - 캡슐 충돌체
    // 
    // 기본 방향: Y축 (세로)
    // Collider::Rotation으로 방향 조절 가능
    // ═══════════════════════════════════════════════════════════════

    class CapsuleCollider : public Collider
    {
        REGISTER_COMPONENT(CapsuleCollider)

    private:
        float m_radius = 0.5f;
        float m_height = 2.0f;  // 총 높이 (양쪽 반구 포함)

    public:
        CapsuleCollider() = default;
        virtual ~CapsuleCollider() = default;

    public:
        float GetRadius() const { return m_radius; }
        void SetRadius(float radius);

        float GetHeight() const { return m_height; }
        void SetHeight(float height);

        // PhysX 스타일: 실린더 부분의 절반 높이
        float GetHalfHeight() const 
        { 
            return std::max(0.0f, (m_height - m_radius * 2.0f) * 0.5f); 
        }

        // 캡슐은 월드 회전 무시
        bool IgnoresWorldRotation() const override { return true; }

    protected:
        physx::PxGeometry* CreateGeometry() override;
        void UpdateGeometry() override;
        void UpdateLocalPose() override;  // PhysX 캡슐이 X축 기본이므로 Y축으로 변환

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override { return "CapsuleCollider"; }
    };
}
