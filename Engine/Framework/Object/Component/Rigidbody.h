#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/ComponentFactory.h"
#include <PxPhysicsAPI.h>

namespace engine
{
    class Collider;

    // ═══════════════════════════════════════════════════════════════
    // Rigidbody 타입
    // ═══════════════════════════════════════════════════════════════

    enum class RigidbodyType
    {
        Dynamic,    // 물리 시뮬레이션 대상
        Kinematic,  // 직접 이동, 다른 오브젝트에 영향을 줌
        Static      // 움직이지 않음 (최적화)
    };

    // ═══════════════════════════════════════════════════════════════
    // 힘 적용 모드
    // ═══════════════════════════════════════════════════════════════

    enum class ForceMode
    {
        Force,          // 연속적인 힘 (질량 고려)
        Impulse,        // 즉각적인 충격 (질량 고려)
        Acceleration,   // 가속도 (질량 무시)
        VelocityChange  // 속도 직접 변경 (질량 무시)
    };

    // ═══════════════════════════════════════════════════════════════
    // 이동/회전 제약
    // ═══════════════════════════════════════════════════════════════

    enum class RigidbodyConstraints : uint32_t
    {
        None = 0,
        FreezePositionX = 1 << 0,
        FreezePositionY = 1 << 1,
        FreezePositionZ = 1 << 2,
        FreezeRotationX = 1 << 3,
        FreezeRotationY = 1 << 4,
        FreezeRotationZ = 1 << 5,
        FreezePosition = FreezePositionX | FreezePositionY | FreezePositionZ,
        FreezeRotation = FreezeRotationX | FreezeRotationY | FreezeRotationZ,
        FreezeAll = FreezePosition | FreezeRotation
    };

    inline RigidbodyConstraints operator|(RigidbodyConstraints a, RigidbodyConstraints b)
    {
        return static_cast<RigidbodyConstraints>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline RigidbodyConstraints operator&(RigidbodyConstraints a, RigidbodyConstraints b)
    {
        return static_cast<RigidbodyConstraints>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    // ═══════════════════════════════════════════════════════════════
    // Rigidbody 컴포넌트
    // ═══════════════════════════════════════════════════════════════

    class Rigidbody : public Component
    {
        REGISTER_COMPONENT(Rigidbody)

    private:
        physx::PxRigidActor* m_actor = nullptr;

        // 속성
        RigidbodyType m_type = RigidbodyType::Dynamic;
        float m_mass = 1.0f;
        float m_linearDamping = 0.0f;
        float m_angularDamping = 0.05f;
        bool m_useGravity = false;  // 기본값: 중력 비활성화
        RigidbodyConstraints m_constraints = RigidbodyConstraints::None;

        // 텔레포트 요청
        bool m_hasPendingTeleport = false;
        Vector3 m_teleportPosition;
        Quaternion m_teleportRotation;
        bool m_teleportResetVelocity = true;

    public:
        Rigidbody() = default;
        virtual ~Rigidbody();

    public:
        void Initialize() override;
        void Awake() override;
        void OnDestroy() override;

        // ═══════════════════════════════════════
        // 타입
        // ═══════════════════════════════════════
        
        RigidbodyType GetRigidbodyType() const { return m_type; }
        void SetRigidbodyType(RigidbodyType type);

        bool IsDynamic() const { return m_type == RigidbodyType::Dynamic; }
        bool IsKinematic() const { return m_type == RigidbodyType::Kinematic; }
        bool IsStatic() const { return m_type == RigidbodyType::Static; }

        // ═══════════════════════════════════════
        // 속성
        // ═══════════════════════════════════════
        
        float GetMass() const { return m_mass; }
        void SetMass(float mass);

        float GetLinearDamping() const { return m_linearDamping; }
        void SetLinearDamping(float damping);

        float GetAngularDamping() const { return m_angularDamping; }
        void SetAngularDamping(float damping);

        bool GetUseGravity() const { return m_useGravity; }
        void SetUseGravity(bool useGravity);

        RigidbodyConstraints GetConstraints() const { return m_constraints; }
        void SetConstraints(RigidbodyConstraints constraints);

        // ═══════════════════════════════════════
        // 힘/토크 적용
        // ═══════════════════════════════════════
        
        void AddForce(const Vector3& force, ForceMode mode = ForceMode::Force);
        void AddTorque(const Vector3& torque, ForceMode mode = ForceMode::Force);
        void AddForceAtPosition(const Vector3& force, const Vector3& position, 
                                ForceMode mode = ForceMode::Force);

        // ═══════════════════════════════════════
        // 속도
        // ═══════════════════════════════════════
        
        Vector3 GetLinearVelocity() const;
        void SetLinearVelocity(const Vector3& velocity);

        Vector3 GetAngularVelocity() const;
        void SetAngularVelocity(const Vector3& velocity);

        // ═══════════════════════════════════════
        // Kinematic 이동
        // ═══════════════════════════════════════
        
        void MovePosition(const Vector3& position);
        void MoveRotation(const Quaternion& rotation);

        // ═══════════════════════════════════════
        // 텔레포트 (Dynamic용)
        // ═══════════════════════════════════════
        
        void Teleport(const Vector3& position, bool resetVelocity = true);
        void Teleport(const Vector3& position, const Quaternion& rotation, bool resetVelocity = true);
        
        bool HasPendingTeleport() const { return m_hasPendingTeleport; }
        void ApplyPendingTeleport();

        // ═══════════════════════════════════════
        // Sleep 상태
        // ═══════════════════════════════════════
        
        bool IsSleeping() const;
        void WakeUp();
        void Sleep();

        // ═══════════════════════════════════════
        // PhysX 접근
        // ═══════════════════════════════════════
        
        physx::PxRigidActor* GetPxActor() const { return m_actor; }

        // ═══════════════════════════════════════
        // 직렬화
        // ═══════════════════════════════════════
        
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override { return "Rigidbody"; }

    private:
        void CreatePxActor();
        void UpdatePxActorProperties();
        void UpdateConstraints();
        void NotifyCollidersAttached();
        void NotifyCollidersDetached();

        physx::PxForceMode::Enum ToPxForceMode(ForceMode mode) const;
    };
}
