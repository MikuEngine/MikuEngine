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
    }

    void Collider::Awake()
    {
        Component::Awake();

        // Shape 생성
        CreateShape();

        if (!m_shape)
        {
            LOG_ERROR("[Collider] Failed to create shape");
            return;
        }

        // 같은 GameObject에 Rigidbody가 있는지 확인
        m_attachedRigidbody = GetGameObject()->GetComponent<Rigidbody>();

        if (m_attachedRigidbody)
        {
            // Rigidbody의 Actor에 Shape 부착
            AttachToRigidbody();
        }
        else
        {
            // 독립 Static Actor 생성
            CreateOwnedStaticActor();
        }

        // PhysicsSystem에 등록
        PhysicsSystem::Get().RegisterCollider(this);
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
    }

    void Collider::Save(json& j) const
    {
        Object::Save(j);  // Type 필드 저장
        j["center"] = { m_center.x, m_center.y, m_center.z };
        j["rotation"] = { m_rotation.x, m_rotation.y, m_rotation.z };
        j["isTrigger"] = m_isTrigger;
        j["layer"] = m_layer;
        j["collisionMask"] = m_collisionMask;
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
        if (j.contains("layer"))
        {
            m_layer = j["layer"].get<uint32_t>();
        }
        if (j.contains("collisionMask"))
        {
            m_collisionMask = j["collisionMask"].get<uint32_t>();
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
            return;
        }

        physx::PxRigidActor* actor = m_attachedRigidbody->GetPxActor();
        if (actor)
        {
            actor->attachShape(*m_shape);
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

        // Scene에 추가
        physx::PxScene* pxScene = PhysicsSystem::Get().GetActivePxScene();
        if (pxScene)
        {
            pxScene->addActor(*m_ownedStaticActor);
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

        // 오일러 각도(degrees)를 쿼터니언으로 변환
        Vector3 radians = m_rotation * (DirectX::XM_PI / 180.0f);
        Quaternion quat = Quaternion::CreateFromYawPitchRoll(radians.y, radians.x, radians.z);

        physx::PxTransform localPose(
            PhysicsUtility::ToPxVec3(m_center),
            physx::PxQuat(quat.x, quat.y, quat.z, quat.w)
        );
        m_shape->setLocalPose(localPose);
    }
}
