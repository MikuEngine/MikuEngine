#include "EnginePCH.h"
#include "Collider.h"

#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Physics/CollisionSystem.h"
#include "Framework/Scene/SceneManager.h"

namespace engine
{
    Collider::~Collider()
    {
        // OnDestroy에서 정리됨
    }

    void Collider::Initialize()
    {
        Component::Initialize();
        // m_lastSyncedScale은 Awake()에서 설정됨
    }

    void Collider::Awake()
    {
        Component::Awake();
        
        // Shape 생성 (m_size 기반, 기본 스케일 1,1,1 상태)
        m_lastSyncedScale = Vector3::One;
        CreateShape();

        if (!m_shape)
        {
            LOG_ERROR("[Collider] Failed to create shape");
            return;
        }

        // 같은 GameObject에 Rigidbody가 있는지 확인
        Rigidbody* rb = GetGameObject()->GetComponent<Rigidbody>();

        if (rb && rb->GetPxActor())
        {
            // Rigidbody가 이미 초기화되어 있으면 바로 부착
            m_attachedRigidbody = rb;
            AttachToRigidbody();
        }
        else if (rb)
        {
            // Rigidbody는 있지만 아직 PxActor가 없음 (Awake 순서 문제)
            // Rigidbody::Awake()에서 NotifyCollidersAttached()가 호출될 때 처리됨
            // 여기서는 m_attachedRigidbody를 설정하지 않음!
            // 대신 독립 Static Actor를 임시로 생성
            CreateOwnedStaticActor();
        }
        else
        {
            // Rigidbody 없음 - 독립 Static Actor 생성
            CreateOwnedStaticActor();
        }

        // PhysicsSystem에 등록
        PhysicsSystem::Get().RegisterCollider(this);

        // WorldScale 적용 (부모 스케일 포함)
        if (m_syncWithTransform && m_shape && GetTransform())
        {
            Vector3 worldScale = GetTransform()->GetWorldScale();
            // 기준 스케일(1,1,1)에서 WorldScale로 변환
            ApplyScaleRatio(worldScale);
            UpdateGeometry();
            m_lastSyncedScale = worldScale;
        }
    }

    void Collider::OnDestroy()
    {
        // CollisionSystem에서 정리
        CollisionSystem::Get().OnColliderDestroyed(this);

        // PhysicsSystem에서 해제
        PhysicsSystem::Get().UnregisterCollider(this);

        // Shape 정리
        if (m_shape)
        {
            if (m_attachedRigidbody)
            {
                physx::PxRigidActor* actor = m_attachedRigidbody->GetPxActor();
                if (actor)
                {
                    actor->detachShape(*m_shape);
                }
            }
            else if (m_ownedStaticActor)
            {
                m_ownedStaticActor->detachShape(*m_shape);
            }

            m_shape->release();
            m_shape = nullptr;
        }

        // Static Actor 정리
        if (m_ownedStaticActor)
        {
            physx::PxScene* pxScene = PhysicsSystem::Get().GetActivePxScene();
            if (pxScene)
            {
                pxScene->removeActor(*m_ownedStaticActor);
            }
            m_ownedStaticActor->release();
            m_ownedStaticActor = nullptr;
        }

        Component::OnDestroy();
    }

    // ═══════════════════════════════════════════════════════════════
    // 속성
    // ═══════════════════════════════════════════════════════════════

    void Collider::SetCenter(const Vector3& center)
    {
        m_center = center;
        UpdateLocalPose();
    }

    void Collider::SetRotation(const Vector3& rotation)
    {
        m_rotation = rotation;
        UpdateLocalPose();
    }

    void Collider::SetIsTrigger(bool isTrigger)
    {
        m_isTrigger = isTrigger;

        if (m_shape)
        {
            if (m_isTrigger)
            {
                m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
                m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
            }
            else
            {
                m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, false);
                m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
            }
        }
    }

    void Collider::SetLayer(uint32_t layer)
    {
        m_layer = layer;
        // PhysicsLayerMatrix에서 자동으로 충돌 마스크 가져오기
        m_collisionMask = PhysicsSystem::Get().GetLayerMatrix().GetCollisionMask(layer);
        UpdateFilterData();
    }

    void Collider::SetCollisionMask(uint32_t mask)
    {
        m_collisionMask = mask;
        UpdateFilterData();
    }

    // ═══════════════════════════════════════════════════════════════
    // Rigidbody 연결
    // ═══════════════════════════════════════════════════════════════

    void Collider::OnRigidbodyAttached(Rigidbody* rb)
    {
        if (m_attachedRigidbody)
        {
            return; // 이미 연결됨
        }

        m_attachedRigidbody = rb;

        if (!m_shape)
        {
            return;
        }

        // 기존 Static Actor에서 Shape 분리
        if (m_ownedStaticActor)
        {
            m_ownedStaticActor->detachShape(*m_shape);

            // Scene에서 제거 후 해제
            physx::PxScene* pxScene = PhysicsSystem::Get().GetActivePxScene();
            if (pxScene)
            {
                pxScene->removeActor(*m_ownedStaticActor);
            }
            m_ownedStaticActor->release();
            m_ownedStaticActor = nullptr;
        }

        // Rigidbody의 Actor에 Shape 부착
        AttachToRigidbody();
    }

    void Collider::OnRigidbodyDetached()
    {
        if (!m_attachedRigidbody)
        {
            return;
        }

        if (m_shape)
        {
            // Rigidbody Actor에서 Shape 분리
            physx::PxRigidActor* actor = m_attachedRigidbody->GetPxActor();
            if (actor)
            {
                actor->detachShape(*m_shape);
            }
        }

        m_attachedRigidbody = nullptr;
    }

    void Collider::OnParentChanged()
    {
        if (!m_shape)
        {
            return;
        }

        // 1. 기존 Rigidbody에서 분리
        if (m_attachedRigidbody)
        {
            physx::PxRigidActor* actor = m_attachedRigidbody->GetPxActor();
            if (actor)
            {
                actor->detachShape(*m_shape);
            }
            m_attachedRigidbody = nullptr;
        }
        else if (m_ownedStaticActor)
        {
            // 독립 Static Actor 정리
            m_ownedStaticActor->detachShape(*m_shape);
            physx::PxScene* pxScene = PhysicsSystem::Get().GetActivePxScene();
            if (pxScene)
            {
                pxScene->removeActor(*m_ownedStaticActor);
            }
            m_ownedStaticActor->release();
            m_ownedStaticActor = nullptr;
        }

        // 2. 새로운 부모 계층에서 Rigidbody 탐색
        Rigidbody* newRigidbody = nullptr;
        
        // 자신의 GameObject에서 Rigidbody 확인
        newRigidbody = GetGameObject()->GetComponent<Rigidbody>();
        
        // 없으면 부모 계층에서 탐색
        if (!newRigidbody)
        {
            Transform* parent = GetTransform()->GetParent();
            while (parent && !newRigidbody)
            {
                newRigidbody = parent->GetGameObject()->GetComponent<Rigidbody>();
                parent = parent->GetParent();
            }
        }

        // 3. 새로운 Rigidbody에 부착 또는 독립 Static Actor 생성
        if (newRigidbody)
        {
            m_attachedRigidbody = newRigidbody;
            AttachToRigidbody();
        }
        else
        {
            CreateOwnedStaticActor();
        }

        // 4. LocalPose 업데이트 (Transform Local 좌표 반영)
        UpdateLocalPose();

        // 독립 Static Actor 생성
        CreateOwnedStaticActor();
    }

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════

    void Collider::OnGui()
    {
        // Center (로컬 오프셋)
        Vector3 center = m_center;
        if (ImGui::DragFloat3("Center", &center.x, 0.01f))
        {
            SetCenter(center);
        }

        // Rotation (로컬 회전)
        Vector3 rotation = m_rotation;
        if (ImGui::DragFloat3("Rotation", &rotation.x, 0.5f, -360.0f, 360.0f))
        {
            SetRotation(rotation);
        }

        // Trigger
        bool isTrigger = m_isTrigger;
        if (ImGui::Checkbox("Is Trigger", &isTrigger))
        {
            SetIsTrigger(isTrigger);
        }

        // Transform 스케일 동기화
        bool syncWithTransform = m_syncWithTransform;
        if (ImGui::Checkbox("Sync With Transform", &syncWithTransform))
        {
            SetSyncWithTransform(syncWithTransform);
            if (syncWithTransform)
            {
                // 동기화 활성화 시 현재 WorldScale로 초기화
                m_lastSyncedScale = GetTransform()->GetWorldScale();
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Enable to automatically sync collider size with Transform scale");
        }

        // Physics Layer 콤보박스
        const char* layerNames[] = {
            "Default", "Player", "Enemy", "Projectile", 
            "Environment", "Trigger", "Layer6", "Layer7"
        };
        int currentLayer = static_cast<int>(m_layer);
        if (currentLayer >= IM_ARRAYSIZE(layerNames)) currentLayer = 0;
        
        if (ImGui::Combo("Physics Layer", &currentLayer, layerNames, IM_ARRAYSIZE(layerNames)))
        {
            SetLayer(static_cast<uint32_t>(currentLayer));
        }
    }

    void Collider::Save(json& j) const
    {
        Object::Save(j);  // Type 필드 저장
        j["center"] = { m_center.x, m_center.y, m_center.z };
        j["rotation"] = { m_rotation.x, m_rotation.y, m_rotation.z };
        j["isTrigger"] = m_isTrigger;
        j["syncWithTransform"] = m_syncWithTransform;
        j["layer"] = m_layer;
        // collisionMask는 저장하지 않음 (PhysicsLayerMatrix에서 자동 설정)
    }

    void Collider::Load(const json& j)
    {
        if (j.contains("center"))
        {
            auto& c = j["center"];
            m_center = Vector3(c[0].get<float>(), c[1].get<float>(), c[2].get<float>());
        }
        if (j.contains("rotation"))
        {
            auto& r = j["rotation"];
            m_rotation = Vector3(r[0].get<float>(), r[1].get<float>(), r[2].get<float>());
        }
        if (j.contains("isTrigger"))
        {
            m_isTrigger = j["isTrigger"].get<bool>();
        }
        if (j.contains("syncWithTransform"))
        {
            m_syncWithTransform = j["syncWithTransform"].get<bool>();
        }
        if (j.contains("layer"))
        {
            m_layer = j["layer"].get<uint32_t>();
            // PhysicsLayerMatrix에서 자동으로 충돌 마스크 가져오기
            m_collisionMask = PhysicsSystem::Get().GetLayerMatrix().GetCollisionMask(m_layer);
        }
        // collisionMask는 더 이상 씬에서 로드하지 않음 (PhysicsLayerMatrix에서 자동 설정)
    }

    void Collider::CheckAndSyncTransformScale()
    {
        if (!m_syncWithTransform || !m_shape)
        {
            return;
        }

        // WorldScale 사용 (부모 스케일 포함)
        Vector3 currentScale = GetTransform()->GetWorldScale();
        
        // 스케일 변경 감지
        const float epsilon = 0.0001f;
        bool scaleChanged = 
            std::abs(currentScale.x - m_lastSyncedScale.x) > epsilon ||
            std::abs(currentScale.y - m_lastSyncedScale.y) > epsilon ||
            std::abs(currentScale.z - m_lastSyncedScale.z) > epsilon;

        if (scaleChanged)
        {
            // 스케일 비율 계산
            Vector3 scaleRatio(
                m_lastSyncedScale.x != 0.0f ? currentScale.x / m_lastSyncedScale.x : 1.0f,
                m_lastSyncedScale.y != 0.0f ? currentScale.y / m_lastSyncedScale.y : 1.0f,
                m_lastSyncedScale.z != 0.0f ? currentScale.z / m_lastSyncedScale.z : 1.0f
            );

            // 파생 클래스에서 스케일 적용 (가상 함수)
            ApplyScaleRatio(scaleRatio);

            // 마지막 동기화 스케일 업데이트
            m_lastSyncedScale = currentScale;

            // Geometry 업데이트
            UpdateGeometry();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Protected 헬퍼
    // ═══════════════════════════════════════════════════════════════

    void Collider::CreateShape()
    {
        physx::PxPhysics* physics = PhysicsSystem::Get().GetPxPhysics();
        if (!physics)
        {
            return;
        }

        // 파생 클래스가 Geometry 생성
        std::unique_ptr<physx::PxGeometry> geometry(CreateGeometry());
        if (!geometry)
        {
            return;
        }

        // 기본 재질 사용
        physx::PxMaterial* material = PhysicsSystem::Get().GetDefaultMaterial();
        if (!material)
        {
            return;
        }

        // Shape 생성
        m_shape = physics->createShape(*geometry, *material, true);
        if (!m_shape)
        {
            return;
        }

        // 로컬 오프셋 및 회전 설정
        UpdateLocalPose();

        // Trigger 설정
        if (m_isTrigger)
        {
            m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
            m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
        }
        else
        {
            // 일반 Collider: 시뮬레이션 활성화
            m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
            m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, false);
        }

        // 쿼리 활성화
        m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        // 필터 데이터 설정
        UpdateFilterData();

        // userData 설정
        m_shape->userData = this;
    }

    void Collider::AttachToRigidbody()
    {
        if (!m_shape || !m_attachedRigidbody)
        {
            LOG_ERROR("[Collider] AttachToRigidbody failed: shape or rigidbody is null");
            return;
        }

        physx::PxRigidActor* actor = m_attachedRigidbody->GetPxActor();
        if (actor)
        {
            // Shape 플래그 확인 (부착 전)
            physx::PxShapeFlags flagsBefore = m_shape->getFlags();
            bool isSimBefore = flagsBefore.isSet(physx::PxShapeFlag::eSIMULATION_SHAPE);
            bool isTriggerBefore = flagsBefore.isSet(physx::PxShapeFlag::eTRIGGER_SHAPE);
            
            actor->attachShape(*m_shape);
            
            // 필터 데이터 재설정 (Shape 부착 후)
            UpdateFilterData();
            
            // Shape 플래그 확인 (부착 후)
            physx::PxShapeFlags flagsAfter = m_shape->getFlags();
            bool isSimAfter = flagsAfter.isSet(physx::PxShapeFlag::eSIMULATION_SHAPE);
            bool isTriggerAfter = flagsAfter.isSet(physx::PxShapeFlag::eTRIGGER_SHAPE);
            
            // const char* rbType = m_attachedRigidbody->IsDynamic() ? "Dynamic" : 
            //     (m_attachedRigidbody->IsKinematic() ? "Kinematic" : "Static");
            // const char* triggerStr = m_isTrigger ? "true" : "false";
            // LOG_PRINT("[Collider] Shape attached to {} Rigidbody (trigger={}, layer={}, mask=0x{:X})", 
            //     rbType, triggerStr, m_layer, m_collisionMask);
            // LOG_PRINT("[Collider] Shape flags: SIMULATION={}->{}, TRIGGER={}->{}, flags=0x{:X}", 
            //     isSimBefore, isSimAfter, isTriggerBefore, isTriggerAfter, 
            //     static_cast<unsigned int>(flagsAfter));
            
            // Actor가 이미 씬에 등록되어 있으면 Shape 부착을 반영하기 위해 다시 등록
            physx::PxScene* scene = actor->getScene();
            if (scene)
            {
                // Actor를 씬에서 제거했다가 다시 추가하여 Shape 변경사항 반영
                scene->removeActor(*actor);
                scene->addActor(*actor);
                // LOG_PRINT("[Collider] Actor re-added to scene after shape attachment: shapes={}", 
                //     actor->getNbShapes());
            }
        }
        else
        {
            LOG_ERROR("[Collider] AttachToRigidbody failed: PxActor is null");
        }
    }

    void Collider::CreateOwnedStaticActor()
    {
        if (!m_shape)
        {
            return;
        }

        physx::PxPhysics* physics = PhysicsSystem::Get().GetPxPhysics();
        if (!physics)
        {
            return;
        }

        // Static Actor 생성
        physx::PxTransform pxTransform;
        if (IgnoresWorldRotation())
        {
            // 월드 회전 무시 - 위치만 사용
            Vector3 worldPos = GetTransform()->GetWorldPosition();
            pxTransform = physx::PxTransform(PhysicsUtility::ToPxVec3(worldPos));
        }
        else
        {
            // 위치와 회전 모두 사용
            pxTransform = PhysicsUtility::ToPxTransform(GetTransform());
        }
        m_ownedStaticActor = physics->createRigidStatic(pxTransform);

        if (!m_ownedStaticActor)
        {
            return;
        }

        // userData 설정 (Collider 포인터)
        m_ownedStaticActor->userData = this;

        // Shape 부착
        m_ownedStaticActor->attachShape(*m_shape);
        
        // Shape 플래그 확인
        physx::PxShapeFlags flags = m_shape->getFlags();
        bool isSim = flags.isSet(physx::PxShapeFlag::eSIMULATION_SHAPE);
        bool isTrigger = flags.isSet(physx::PxShapeFlag::eTRIGGER_SHAPE);
        LOG_PRINT("[Collider] Static Actor shape flags: SIMULATION={}, TRIGGER={}, flags=0x{:X}", 
            isSim, isTrigger, static_cast<unsigned int>(flags));

        // Scene에 추가
        physx::PxScene* pxScene = PhysicsSystem::Get().GetActivePxScene();
        if (pxScene)
        {
            pxScene->addActor(*m_ownedStaticActor);
            LOG_PRINT("[Collider] Static Actor added to scene: layer={}, mask=0x{:X}, shapes={}", 
                m_layer, m_collisionMask, m_ownedStaticActor->getNbShapes());
        }
        else
        {
            LOG_ERROR("[Collider] Failed to add Static Actor to scene: scene is null");
        }
    }

    void Collider::UpdateFilterData()
    {
        if (!m_shape)
        {
            return;
        }

        physx::PxFilterData filterData;
        filterData.word0 = m_layer;         // 자신의 레이어
        filterData.word1 = m_collisionMask; // 충돌할 레이어 마스크
        filterData.word2 = 0;
        filterData.word3 = 0;

        m_shape->setSimulationFilterData(filterData);
        m_shape->setQueryFilterData(filterData);
    }

    void Collider::UpdateLocalPose()
    {
        if (!m_shape)
        {
            return;
        }

        // 기본 오프셋: Collider의 center
        Vector3 totalOffset = m_center;
        Quaternion totalRotation;

        // 오일러 각도(degrees)를 쿼터니언으로 변환 (Collider 자체 회전)
        Vector3 radians = m_rotation * (DirectX::XM_PI / 180.0f);
        totalRotation = Quaternion::CreateFromYawPitchRoll(radians.y, radians.x, radians.z);

        // 부모 Rigidbody가 있는 경우, Transform의 Local 좌표를 Shape LocalPose에 반영
        if (m_attachedRigidbody)
        {
            // Rigidbody가 같은 GameObject에 있는지 확인
            bool isSameGameObject = (m_attachedRigidbody->GetGameObject() == GetGameObject());
            
            if (!isSameGameObject)
            {
                // 다른 GameObject (부모 계층의 Rigidbody)
                // Rigidbody의 Transform 기준으로 상대 위치 계산
                Transform* rbTransform = m_attachedRigidbody->GetTransform();
                Transform* myTransform = GetTransform();
                
                // 내 World Position - Rigidbody의 World Position = 상대 오프셋
                Vector3 rbWorldPos = rbTransform->GetWorldPosition();
                Vector3 myWorldPos = myTransform->GetWorldPosition();
                Vector3 relativeOffset = myWorldPos - rbWorldPos;
                
                // 총 오프셋 = 상대 오프셋 + Collider center
                totalOffset = relativeOffset + m_center;
                
                // 회전도 상대적으로 계산 (필요 시)
                // 현재는 간단히 Collider 자체 회전만 사용
            }
        }

        physx::PxTransform localPose(
            PhysicsUtility::ToPxVec3(totalOffset),
            physx::PxQuat(totalRotation.x, totalRotation.y, totalRotation.z, totalRotation.w)
        );
        m_shape->setLocalPose(localPose);
    }
}
