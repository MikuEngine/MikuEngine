#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Physics/PhysicsLayer.h"
#include "Framework/Physics/CollisionTypes.h"
#include <PxPhysicsAPI.h>

namespace engine
{
    class Rigidbody;
    class PhysicsMaterial;

    // ═══════════════════════════════════════════════════════════════
    // Collider 기반 클래스
    // 
    // - Rigidbody가 있으면: Rigidbody의 PxActor에 Shape 부착
    // - Rigidbody가 없으면: 자체 PxRigidStatic 생성 (정적 충돌체)
    // ═══════════════════════════════════════════════════════════════

    class Collider : public Component
    {
        DEFINE_COMPONENT_TYPE(Collider, Component)

    protected:
        physx::PxShape* m_shape = nullptr;
        
        // Rigidbody가 없을 때 자동 생성되는 Static Actor
        physx::PxRigidStatic* m_ownedStaticActor = nullptr;
        
        // 연결된 Rigidbody (있으면)
        Rigidbody* m_attachedRigidbody = nullptr;

        // 속성
        Vector3 m_center{ 0.0f, 0.0f, 0.0f };   // 로컬 오프셋
        Vector3 m_rotation{ 0.0f, 0.0f, 0.0f }; // 로컬 회전 (오일러 각도, degrees)
        bool m_isTrigger = false;
        bool m_syncWithTransform = true;        // Transform 스케일과 자동 동기화
        Vector3 m_lastSyncedScale{ 1.0f, 1.0f, 1.0f };  // 마지막 동기화된 스케일
        
        // 레이어
        uint32_t m_layer = PhysicsLayer::Default;
        uint32_t m_collisionMask = PhysicsLayer::Mask::All;
        
        // 우선순위 (전투 시스템용)
        CollisionPriority m_collisionPriority = CollisionPriority::Default;
        uint64_t m_attackSourceId = 0;

    public:
        Collider() = default;
        virtual ~Collider();

    public:
        virtual void Initialize() override;
        virtual void Awake() override;
        virtual void OnDestroy() override;

        // ═══════════════════════════════════════
        // 속성
        // ═══════════════════════════════════════
        
        // 로컬 오프셋
        const Vector3& GetCenter() const { return m_center; }
        void SetCenter(const Vector3& center);

        // 로컬 회전 (오일러 각도, degrees)
        const Vector3& GetRotation() const { return m_rotation; }
        void SetRotation(const Vector3& rotation);

        // Trigger 모드
        bool IsTrigger() const { return m_isTrigger; }
        void SetIsTrigger(bool isTrigger);

        // Transform 스케일 동기화
        bool IsSyncWithTransform() const { return m_syncWithTransform; }
        void SetSyncWithTransform(bool sync) { m_syncWithTransform = sync; }
        void CheckAndSyncTransformScale();  // Transform 스케일 변경 체크 및 동기화

        // 레이어
        uint32_t GetLayer() const { return m_layer; }
        void SetLayer(uint32_t layer);
        
        uint32_t GetCollisionMask() const { return m_collisionMask; }
        void SetCollisionMask(uint32_t mask);

        // 우선순위 (전투 시스템용)
        CollisionPriority GetCollisionPriority() const { return m_collisionPriority; }
        void SetCollisionPriority(CollisionPriority priority) { m_collisionPriority = priority; }
        
        uint64_t GetAttackSourceId() const { return m_attackSourceId; }
        void SetAttackSourceId(uint64_t id) { m_attackSourceId = id; }

        // 월드 회전 무시 여부 (Sphere, Capsule은 true)
        virtual bool IgnoresWorldRotation() const { return false; }

        // ═══════════════════════════════════════
        // Rigidbody 연결
        // ═══════════════════════════════════════
        
        bool HasRigidbody() const { return m_attachedRigidbody != nullptr; }
        Rigidbody* GetAttachedRigidbody() const { return m_attachedRigidbody; }

        // Rigidbody가 추가/제거될 때 호출 (Rigidbody에서 호출)
        void OnRigidbodyAttached(Rigidbody* rb);
        void OnRigidbodyDetached();

        // 부모 Transform이 변경될 때 호출 (Transform에서 호출)
        void OnParentChanged();

        // ═══════════════════════════════════════
        // PhysX 접근
        // ═══════════════════════════════════════
        
        physx::PxShape* GetPxShape() const { return m_shape; }
        physx::PxRigidStatic* GetOwnedStaticActor() const { return m_ownedStaticActor; }

        // ═══════════════════════════════════════
        // 직렬화
        // ═══════════════════════════════════════
        
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    protected:
        // 파생 클래스에서 구현
        virtual physx::PxGeometry* CreateGeometry() = 0;
        virtual void UpdateGeometry() = 0;
        virtual void ApplyScaleRatio(const Vector3& scaleRatio) {}  // 스케일 비율 적용

        // 공통 초기화
        void CreateShape();
        void AttachToRigidbody();
        void CreateOwnedStaticActor();
        void UpdateFilterData();
        virtual void UpdateLocalPose();  // center와 rotation을 shape에 적용
    };
}
