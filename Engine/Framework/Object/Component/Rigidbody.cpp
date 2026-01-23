#include "EnginePCH.h"
#include "Rigidbody.h"

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/System/SystemManager.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

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

        // Scene에 등록
        if (Scene* scene = SceneManager::Get().GetScene())
        {
            scene->RegisterRigidbody(this);
        }

        // Collider들에게 알림
        NotifyCollidersAttached();
    }

    void Rigidbody::OnDestroy()
    {
        // Collider들에게 분리 알림
        NotifyCollidersDetached();

        // Scene에서 해제
        Scene* scene = SceneManager::Get().GetScene();
        if (scene)
        {
            scene->UnregisterRigidbody(this);
        }

        // PxActor 해제
        // 주의: PxScene이 release되면 내부 Actor들도 자동 해제됨
        // PxScene이 이미 해제된 경우 m_actor->release() 호출하면 안됨
        if (m_actor)
        {
            // PxScene이 아직 존재하면 Actor를 수동으로 해제
            if (scene && scene->GetPxScene())
            {
                m_actor->release();
            }
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
            if (Scene* scene = SceneManager::Get().GetScene())
            {
                scene->UnregisterRigidbody(this);
            }
            NotifyCollidersDetached();
            m_actor->release();
            m_actor = nullptr;

            // 새 Actor 생성
            CreatePxActor();
            if (m_actor)
            {
                if (Scene* scene = SceneManager::Get().GetScene())
                {
                    scene->RegisterRigidbody(this);
                }
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
    // 참고: PhysX에서 키네마틱 Rigidbody도 PxRigidDynamic을 사용합니다.
    // setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true)로 키네마틱 모드로 설정됩니다.

    void Rigidbody::MovePosition(const Vector3& position)
    {
        if (!m_actor || m_type != RigidbodyType::Kinematic)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            // 현재 회전 유지하고 위치만 변경
            physx::PxTransform currentPose = dynamic->getGlobalPose();
            physx::PxTransform newPose(PhysicsUtility::ToPxVec3(position), currentPose.q);
            
            // setKinematicTarget은 다음 물리 시뮬레이션 스텝에서 적용됨
            // Transform과의 동기화를 위해 Transform도 업데이트해야 할 수 있음
            dynamic->setKinematicTarget(newPose);
            
            // Transform도 즉시 업데이트 (시각적 피드백을 위해)
            if (Transform* transform = GetGameObject()->GetTransform())
            {
                transform->SetLocalPosition(position);
            }
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

            // 씬에 있을 때만 wakeUp 호출
            if (dynamic->getScene())
            {
                dynamic->wakeUp();
            }
        }

        m_hasPendingTeleport = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 강제 Transform 설정
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::ForceSetPosition(const Vector3& position, bool resetVelocity)
    {
        if (!m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxTransform currentPose = dynamic->getGlobalPose();
            currentPose.p = PhysicsUtility::ToPxVec3(position);
            dynamic->setGlobalPose(currentPose);

            if (resetVelocity)
            {
                dynamic->setLinearVelocity(physx::PxVec3(0));
            }

            // 씬에 있을 때만 wakeUp 호출
            if (dynamic->getScene())
            {
                dynamic->wakeUp();
            }
        }

        // Transform도 동기화 (World → Local 변환)
        Transform* transform = GetTransform();
        Transform* parent = transform->GetParent();
        if (parent)
        {
            // 부모의 역변환 적용
            Vector3 localPos = position - parent->GetWorldPosition();
            // TODO: 부모 회전/스케일 고려 필요 시 추가 구현
            transform->SetLocalPosition(localPos);
        }
        else
        {
            transform->SetLocalPosition(position);
        }
    }

    void Rigidbody::ForceSetRotation(const Quaternion& rotation, bool resetAngularVelocity)
    {
        if (!m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxTransform currentPose = dynamic->getGlobalPose();
            currentPose.q = PhysicsUtility::ToPxQuat(rotation);
            dynamic->setGlobalPose(currentPose);

            if (resetAngularVelocity)
            {
                dynamic->setAngularVelocity(physx::PxVec3(0));
            }

            // 씬에 있을 때만 wakeUp 호출
            if (dynamic->getScene())
            {
                dynamic->wakeUp();
            }
        }

        // Transform도 동기화 (World → Local 변환)
        // 부모가 없으면 Local = World
        Transform* transform = GetTransform();
        Transform* parent = transform->GetParent();
        if (parent)
        {
            // 부모의 역회전 적용
            // TODO: 정확한 역변환 필요 시 추가 구현
            transform->SetLocalRotation(rotation);
        }
        else
        {
            transform->SetLocalRotation(rotation);
        }
    }

    void Rigidbody::ForceSetTransform(const Vector3& position, const Quaternion& rotation,
                                       bool resetVelocity, bool resetAngularVelocity)
    {
        if (!m_actor)
        {
            return;
        }

        physx::PxRigidDynamic* dynamic = m_actor->is<physx::PxRigidDynamic>();
        if (dynamic)
        {
            physx::PxTransform newPose = PhysicsUtility::ToPxTransform(position, rotation);
            dynamic->setGlobalPose(newPose);

            if (resetVelocity)
            {
                dynamic->setLinearVelocity(physx::PxVec3(0));
            }

            if (resetAngularVelocity)
            {
                dynamic->setAngularVelocity(physx::PxVec3(0));
            }

            // 씬에 있을 때만 wakeUp 호출
            if (dynamic->getScene())
            {
                dynamic->wakeUp();
            }
        }

        // Transform도 동기화 (World → Local 변환)
        Transform* transform = GetTransform();
        Transform* parent = transform->GetParent();
        if (parent)
        {
            Vector3 localPos = position - parent->GetWorldPosition();
            transform->SetLocalPosition(localPos);
            transform->SetLocalRotation(rotation);
        }
        else
        {
            transform->SetLocalPosition(position);
            transform->SetLocalRotation(rotation);
        }
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
        if (dynamic && dynamic->getScene())
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
        // Body Type 콤보박스
        const char* bodyTypes[] = { "Dynamic", "Kinematic", "Static" };
        int currentType = static_cast<int>(m_type);
        if (ImGui::Combo("Body Type", &currentType, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
        {
            SetRigidbodyType(static_cast<RigidbodyType>(currentType));
        }

        // Dynamic일 때만 의미 있는 속성들
        bool isDynamic = (m_type == RigidbodyType::Dynamic);

        // Use Gravity (Dynamic일 때만 활성화)
        if (!isDynamic) ImGui::BeginDisabled();
        bool useGravity = m_useGravity;
        if (ImGui::Checkbox("Use Gravity", &useGravity))
        {
            SetUseGravity(useGravity);
        }
        if (!isDynamic) ImGui::EndDisabled();

        // Mass (Dynamic일 때만 활성화)
        if (!isDynamic) ImGui::BeginDisabled();
        float mass = m_mass;
        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.001f, 1000.0f))
        {
            SetMass(mass);
        }
        if (!isDynamic) ImGui::EndDisabled();

        // Linear Damping (이동 감쇠: 0=없음, 100=빠른 감쇠)
        float linearDamping = m_linearDamping;
        if (ImGui::DragFloat("Linear Damping", &linearDamping, 1.0f, 0.0f, 1000.0f))
        {
            SetLinearDamping(linearDamping);
        }

        // Angular Damping (회전 감쇠: 0=없음, 100=빠른 감쇠)
        float angularDamping = m_angularDamping;
        if (ImGui::DragFloat("Angular Damping", &angularDamping, 1.0f, 0.0f, 1000.0f))
        {
            SetAngularDamping(angularDamping);
        }

        // Constraints (접이식)
        if (ImGui::TreeNode("Constraints"))
        {
            uint32_t constraints = static_cast<uint32_t>(m_constraints);
            
            ImGui::Text("Freeze Position");
            bool freezePosX = (constraints & static_cast<uint32_t>(RigidbodyConstraints::FreezePositionX)) != 0;
            bool freezePosY = (constraints & static_cast<uint32_t>(RigidbodyConstraints::FreezePositionY)) != 0;
            bool freezePosZ = (constraints & static_cast<uint32_t>(RigidbodyConstraints::FreezePositionZ)) != 0;
            
            if (ImGui::Checkbox("X##PosX", &freezePosX)) 
            {
                constraints ^= static_cast<uint32_t>(RigidbodyConstraints::FreezePositionX);
                SetConstraints(static_cast<RigidbodyConstraints>(constraints));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Y##PosY", &freezePosY))
            {
                constraints ^= static_cast<uint32_t>(RigidbodyConstraints::FreezePositionY);
                SetConstraints(static_cast<RigidbodyConstraints>(constraints));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Z##PosZ", &freezePosZ))
            {
                constraints ^= static_cast<uint32_t>(RigidbodyConstraints::FreezePositionZ);
                SetConstraints(static_cast<RigidbodyConstraints>(constraints));
            }

            ImGui::Text("Freeze Rotation");
            bool freezeRotX = (constraints & static_cast<uint32_t>(RigidbodyConstraints::FreezeRotationX)) != 0;
            bool freezeRotY = (constraints & static_cast<uint32_t>(RigidbodyConstraints::FreezeRotationY)) != 0;
            bool freezeRotZ = (constraints & static_cast<uint32_t>(RigidbodyConstraints::FreezeRotationZ)) != 0;
            
            if (ImGui::Checkbox("X##RotX", &freezeRotX))
            {
                constraints ^= static_cast<uint32_t>(RigidbodyConstraints::FreezeRotationX);
                SetConstraints(static_cast<RigidbodyConstraints>(constraints));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Y##RotY", &freezeRotY))
            {
                constraints ^= static_cast<uint32_t>(RigidbodyConstraints::FreezeRotationY);
                SetConstraints(static_cast<RigidbodyConstraints>(constraints));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Z##RotZ", &freezeRotZ))
            {
                constraints ^= static_cast<uint32_t>(RigidbodyConstraints::FreezeRotationZ);
                SetConstraints(static_cast<RigidbodyConstraints>(constraints));
            }

            ImGui::TreePop();
        }
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
    }

    // ═══════════════════════════════════════════════════════════════
    // Private
    // ═══════════════════════════════════════════════════════════════

    void Rigidbody::CreatePxActor()
    {
        physx::PxPhysics* physics = SystemManager::Get().GetPhysicsSystem().GetPxPhysics();
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
                else
                {
                    // Dynamic: Sleep threshold를 0으로 설정 (항상 깨어있음)
                    // 주의: wakeUp()은 씬에 추가된 후에만 호출 가능
                    dynamic->setSleepThreshold(0.0f);
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
            if (comp->Is<Collider>())
            {
                static_cast<Collider*>(comp.get())->OnRigidbodyAttached(this);
            }
        }
    }

    void Rigidbody::NotifyCollidersDetached()
    {
        const auto& components = GetGameObject()->GetComponents();
        for (const auto& comp : components)
        {
            if (comp->Is<Collider>())
            {
                static_cast<Collider*>(comp.get())->OnRigidbodyDetached();
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
