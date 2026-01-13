#include "EnginePCH.h"
#include "Rigidbody.h"

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/PhysicsUtility.h"

namespace engine
{
    Rigidbody::~Rigidbody()
    {
        // OnDestroy에서 정리됨
    }

    void Rigidbody::Initialize()
    {
        Component::Initialize();
    }

    void Rigidbody::Awake()
    {
        Component::Awake();

        // PxActor 생성
        CreatePxActor();

        if (!m_actor)
        {
            LOG_ERROR("[Rigidbody] Failed to create PxActor");
            return;
        }

        // PhysicsSystem에 등록
        PhysicsSystem::Get().RegisterRigidbody(this);

        // Collider들에게 알림
        NotifyCollidersAttached();
    }

    void Rigidbody::OnDestroy()
    {
        // Collider들에게 분리 알림
        NotifyCollidersDetached();

        // PhysicsSystem에서 해제
        PhysicsSystem::Get().UnregisterRigidbody(this);

        // PxActor 해제
        if (m_actor)
        {
            m_actor->release();
            m_actor = nullptr;
        }

        Component::OnDestroy();
    }

    // ═══════════════════════════════════════════════════════════════
    // 타입
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::SetRigidbodyType(RigidbodyType type)
    {
        if (m_type == type)
        {
            return;
        }

        m_type = type;

        // 타입 변경 시 Actor 재생성 필요
        if (m_actor)
        {
            // 기존 Actor 해제
            PhysicsSystem::Get().UnregisterRigidbody(this);
            NotifyCollidersDetached();
            m_actor->release();
            m_actor = nullptr;

            // 새 Actor 생성
            CreatePxActor();
            if (m_actor)
            {
                PhysicsSystem::Get().RegisterRigidbody(this);
                NotifyCollidersAttached();
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 속성
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::SetMass(float mass)
    {
        m_mass = std::max(0.001f, mass);

        if (m_actor)
        {
            physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
            if (dynamic)
            {
                dynamic->setMass(m_mass);
            }
        }
    }

    void Rigidbody::SetLinearDamping(float damping)
    {
        m_linearDamping = std::max(0.0f, damping);

        if (m_actor)
        {
            physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
            if (dynamic)
            {
                dynamic->setLinearDamping(m_linearDamping);
            }
        }
    }

    void Rigidbody::SetAngularDamping(float damping)
    {
        m_angularDamping = std::max(0.0f, damping);

        if (m_actor)
        {
            physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
            if (dynamic)
            {
                dynamic->setAngularDamping(m_angularDamping);
            }
        }
    }

    void Rigidbody::SetUseGravity(bool useGravity)
    {
        m_useGravity = useGravity;

        if (m_actor)
        {
            m_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !m_useGravity);
        }
    }

    void Rigidbody::SetConstraints(RigidbodyConstraints constraints)
    {
        m_constraints = constraints;
        UpdateConstraints();
    }

    void Rigidbody::SetLayer(uint32_t layer)
    {
        m_layer = layer;
        // Collider의 필터 데이터도 갱신 필요
    }

    // ═══════════════════════════════════════════════════════════════
    // 힘/토크
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::AddForce(const Vector3& force, ForceMode mode)
    {
        if (!m_actor || m_type != RigidbodyType::Dynamic)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            dynamic->addForce(
                PhysicsUtility::ToPxVec3(force),
                ToPxForceMode(mode)
            );
        }
    }

    void Rigidbody::AddTorque(const Vector3& torque, ForceMode mode)
    {
        if (!m_actor || m_type != RigidbodyType::Dynamic)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            dynamic->addTorque(
                PhysicsUtility::ToPxVec3(torque),
                ToPxForceMode(mode)
            );
        }
    }

    void Rigidbody::AddForceAtPosition(const Vector3& force, const Vector3& position, ForceMode mode)
    {
        if (!m_actor || m_type != RigidbodyType::Dynamic)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxRigidBodyExt::addForceAtPos(
                *dynamic,
                PhysicsUtility::ToPxVec3(force),
                PhysicsUtility::ToPxVec3(position),
                ToPxForceMode(mode)
            );
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 속도
    // ═══════════════════════════════════════════════════════════════

    Vector3 Rigidbody::GetLinearVelocity() const
    {
        if (!m_actor)
        {
            return Vector3::Zero;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            return PhysicsUtility::ToVector3(dynamic->getLinearVelocity());
        }

        return Vector3::Zero;
    }

    void Rigidbody::SetLinearVelocity(const Vector3& velocity)
    {
        if (!m_actor || m_type == RigidbodyType::Static)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            dynamic->setLinearVelocity(PhysicsUtility::ToPxVec3(velocity));
        }
    }

    Vector3 Rigidbody::GetAngularVelocity() const
    {
        if (!m_actor)
        {
            return Vector3::Zero;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            return PhysicsUtility::ToVector3(dynamic->getAngularVelocity());
        }

        return Vector3::Zero;
    }

    void Rigidbody::SetAngularVelocity(const Vector3& velocity)
    {
        if (!m_actor || m_type == RigidbodyType::Static)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            dynamic->setAngularVelocity(PhysicsUtility::ToPxVec3(velocity));
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Kinematic 이동
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::MovePosition(const Vector3& position)
    {
        if (!m_actor || m_type != RigidbodyType::Kinematic)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxTransform currentPose = dynamic->getGlobalPose();
            physx::PxTransform newPose(PhysicsUtility::ToPxVec3(position), currentPose.q);
            dynamic->setKinematicTarget(newPose);
        }
    }

    void Rigidbody::MoveRotation(const Quaternion& rotation)
    {
        if (!m_actor || m_type != RigidbodyType::Kinematic)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxTransform currentPose = dynamic->getGlobalPose();
            physx::PxTransform newPose(currentPose.p, PhysicsUtility::ToPxQuat(rotation));
            dynamic->setKinematicTarget(newPose);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 텔레포트
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::Teleport(const Vector3& position, bool resetVelocity)
    {
        Teleport(position, GetTransform()->GetLocalRotation(), resetVelocity);
    }

    void Rigidbody::Teleport(const Vector3& position, const Quaternion& rotation, bool resetVelocity)
    {
        m_hasPendingTeleport = true;
        m_teleportPosition = position;
        m_teleportRotation = rotation;
        m_teleportResetVelocity = resetVelocity;
    }

    void Rigidbody::ApplyPendingTeleport()
    {
        if (!m_hasPendingTeleport || !m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxTransform newPose = PhysicsUtility::ToPxTransform(
                m_teleportPosition, m_teleportRotation
            );
            dynamic->setGlobalPose(newPose);

            if (m_teleportResetVelocity)
            {
                dynamic->setLinearVelocity(physx::PxVec3(0));
                dynamic->setAngularVelocity(physx::PxVec3(0));
            }

            dynamic->wakeUp();
        }

        m_hasPendingTeleport = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // Sleep
    // ═══════════════════════════════════════════════════════════════

    bool Rigidbody::IsSleeping() const
    {
        if (!m_actor)
        {
            return true;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            return dynamic->isSleeping();
        }

        return true;
    }

    void Rigidbody::WakeUp()
    {
        if (!m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            dynamic->wakeUp();
        }
    }

    void Rigidbody::Sleep()
    {
        if (!m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            dynamic->putToSleep();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::OnGui()
    {
        // TODO: ImGui 편집
    }

    void Rigidbody::Save(json& j) const
    {
        Object::Save(j);  // Type 필드 저장
        j["type"] = static_cast<int>(m_type);
        j["mass"] = m_mass;
        j["linearDamping"] = m_linearDamping;
        j["angularDamping"] = m_angularDamping;
        j["useGravity"] = m_useGravity;
        j["constraints"] = static_cast<uint32_t>(m_constraints);
        j["layer"] = m_layer;
    }

    void Rigidbody::Load(const json& j)
    {
        if (j.contains("type"))
        {
            m_type = static_cast<RigidbodyType>(j["type"].get<int>());
        }
        if (j.contains("mass"))
        {
            m_mass = j["mass"].get<float>();
        }
        if (j.contains("linearDamping"))
        {
            m_linearDamping = j["linearDamping"].get<float>();
        }
        if (j.contains("angularDamping"))
        {
            m_angularDamping = j["angularDamping"].get<float>();
        }
        if (j.contains("useGravity"))
        {
            m_useGravity = j["useGravity"].get<bool>();
        }
        if (j.contains("constraints"))
        {
            m_constraints = static_cast<RigidbodyConstraints>(j["constraints"].get<uint32_t>());
        }
        if (j.contains("layer"))
        {
            m_layer = j["layer"].get<uint32_t>();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Private
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::CreatePxActor()
    {
        physx::PxPhysics* physics = PhysicsSystem::Get().GetPxPhysics();
        if (!physics)
        {
            return;
        }

        physx::PxTransform transform = PhysicsUtility::ToPxTransform(GetTransform());

        switch (m_type)
        {
        case RigidbodyType::Static:
            m_actor = physics->createRigidStatic(transform);
            break;

        case RigidbodyType::Dynamic:
        case RigidbodyType::Kinematic:
            {
                physx::PxRigidDynamic* dynamic = physics->createRigidDynamic(transform);
                
                if (m_type == RigidbodyType::Kinematic)
                {
                    dynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
                }

                dynamic->setMass(m_mass);
                dynamic->setLinearDamping(m_linearDamping);
                dynamic->setAngularDamping(m_angularDamping);

                m_actor = dynamic;
            }
            break;
        }

        if (m_actor)
        {
            // 중력 설정
            m_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !m_useGravity);

            // userData 설정
            m_actor->userData = this;

            // 제약 조건 적용
            UpdateConstraints();
        }
    }

    void Rigidbody::UpdateConstraints()
    {
        if (!m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (!dynamic)
        {
            return;
        }

        physx::PxRigidDynamicLockFlags lockFlags = static_cast<physx::PxRigidDynamicLockFlags>(0);

        if ((m_constraints & RigidbodyConstraints::FreezePositionX) != RigidbodyConstraints::None)
            lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
        if ((m_constraints & RigidbodyConstraints::FreezePositionY) != RigidbodyConstraints::None)
            lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
        if ((m_constraints & RigidbodyConstraints::FreezePositionZ) != RigidbodyConstraints::None)
            lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
        if ((m_constraints & RigidbodyConstraints::FreezeRotationX) != RigidbodyConstraints::None)
            lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        if ((m_constraints & RigidbodyConstraints::FreezeRotationY) != RigidbodyConstraints::None)
            lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        if ((m_constraints & RigidbodyConstraints::FreezeRotationZ) != RigidbodyConstraints::None)
            lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;

        dynamic->setRigidDynamicLockFlags(lockFlags);
    }

    void Rigidbody::NotifyCollidersAttached()
    {
        // 같은 GameObject의 모든 Collider에게 알림
        const auto& components = GetGameObject()->GetComponents();
        for (const auto& comp : components)
        {
            Collider* collider = dynamic_cast<Collider*>(comp.get());
            if (collider && collider != nullptr)
            {
                collider->OnRigidbodyAttached(this);
            }
        }
    }

    void Rigidbody::NotifyCollidersDetached()
    {
        const auto& components = GetGameObject()->GetComponents();
        for (const auto& comp : components)
        {
            Collider* collider = dynamic_cast<Collider*>(comp.get());
            if (collider)
            {
                collider->OnRigidbodyDetached();
            }
        }
    }

    physx::PxForceMode::Enum Rigidbody::ToPxForceMode(ForceMode mode) const
    {
        switch (mode)
        {
        case ForceMode::Force:          return physx::PxForceMode::eFORCE;
        case ForceMode::Impulse:        return physx::PxForceMode::eIMPULSE;
        case ForceMode::Acceleration:   return physx::PxForceMode::eACCELERATION;
        case ForceMode::VelocityChange: return physx::PxForceMode::eVELOCITY_CHANGE;
        default:                        return physx::PxForceMode::eFORCE;
        }
    }
}
