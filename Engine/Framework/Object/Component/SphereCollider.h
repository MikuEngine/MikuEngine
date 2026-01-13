#pragma once

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // SphereCollider - 구 충돌체
    // ═══════════════════════════════════════════════════════════════

    class SphereCollider : public Collider
    {
        REGISTER_COMPONENT(SphereCollider)

    private:
        float m_radius = 0.5f;

    public:
        SphereCollider() = default;
        virtual ~SphereCollider() = default;

    public:
        float GetRadius() const { return m_radius; }
        void SetRadius(float radius);

        // 구체는 월드 회전 무시
        bool IgnoresWorldRotation() const override { return true; }

    protected:
        physx::PxGeometry* CreateGeometry() override;
        void UpdateGeometry() override;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override { return "SphereCollider"; }
    };
}
