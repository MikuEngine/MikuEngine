#pragma once

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // BoxCollider - 직육면체 충돌체
    // ═══════════════════════════════════════════════════════════════

    class BoxCollider : public Collider
    {
        REGISTER_COMPONENT(BoxCollider)

    private:
        Vector3 m_size{ 1.0f, 1.0f, 1.0f };  // 전체 크기 (half extents × 2)

    public:
        BoxCollider() = default;
        virtual ~BoxCollider() = default;

    public:
        // 크기 (전체 크기, half extents 아님)
        const Vector3& GetSize() const { return m_size; }
        void SetSize(const Vector3& size);

        // Half Extents (PhysX 스타일)
        Vector3 GetHalfExtents() const { return m_size * 0.5f; }
        void SetHalfExtents(const Vector3& halfExtents);

    protected:
        physx::PxGeometry* CreateGeometry() override;
        void UpdateGeometry() override;
        void ApplyScaleRatio(const Vector3& scaleRatio) override;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override { return "BoxCollider"; }
    };
}
