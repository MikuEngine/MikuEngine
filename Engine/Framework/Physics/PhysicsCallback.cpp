#include "EnginePCH.h"
#include "PhysicsCallback.h"

#include "Framework/Physics/CollisionSystem.h"
#include "Framework/Physics/PhysicsLayer.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/Rigidbody.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // PhysicsEventCallback 구현
    // ═══════════════════════════════════════════════════════════════

    void PhysicsEventCallback::onContact(
        const physx::PxContactPairHeader& pairHeader,
        const physx::PxContactPair* pairs,
        physx::PxU32 nbPairs)
    {
        // 삭제된 Actor 스킵
        if (pairHeader.flags & (physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 |
                                physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
        {
            return;
        }

        for (physx::PxU32 i = 0; i < nbPairs; ++i)
        {
            const physx::PxContactPair& pair = pairs[i];

            // 삭제된 Shape 스킵
            if (pair.flags & (physx::PxContactPairFlag::eREMOVED_SHAPE_0 |
                              physx::PxContactPairFlag::eREMOVED_SHAPE_1))
            {
                continue;
            }

            // Shape에서 Collider 얻기
            Collider* colliderA = static_cast<Collider*>(pair.shapes[0]->userData);
            Collider* colliderB = static_cast<Collider*>(pair.shapes[1]->userData);

            if (!colliderA || !colliderB)
            {
                continue;
            }

            // 이벤트 타입 판별
            CollisionEventType eventType;
            if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
            {
                eventType = CollisionEventType::Enter;
            }
            else if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
            {
                eventType = CollisionEventType::Stay;
            }
            else if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
            {
                eventType = CollisionEventType::Exit;
            }
            else
            {
                continue;
            }

            // 충돌 이벤트 생성
            CollisionEvent event;
            event.type = eventType;
            event.colliderA = Ptr<Collider>(colliderA);
            event.colliderB = Ptr<Collider>(colliderB);

            // 접촉점 추출 (Exit 제외)
            if (eventType != CollisionEventType::Exit)
            {
                ExtractContactPoints(pair, event.contacts);
            }

            // 우선순위 결정 (콜라이더에서 가져옴)
            CollisionPriority priorityA = colliderA->GetCollisionPriority();
            CollisionPriority priorityB = colliderB->GetCollisionPriority();
            event.priority = (priorityA > priorityB) ? priorityA : priorityB;

            // CollisionSystem에 큐잉
            CollisionSystem::Get().QueueCollisionEvent(event);
        }
    }

    void PhysicsEventCallback::onTrigger(
        physx::PxTriggerPair* pairs,
        physx::PxU32 count)
    {
        for (physx::PxU32 i = 0; i < count; ++i)
        {
            const physx::PxTriggerPair& pair = pairs[i];

            // 삭제된 Shape 스킵
            if (pair.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER |
                              physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
            {
                continue;
            }

            // Shape에서 Collider 얻기
            Collider* trigger = static_cast<Collider*>(pair.triggerShape->userData);
            Collider* other = static_cast<Collider*>(pair.otherShape->userData);

            if (!trigger || !other)
            {
                continue;
            }

            // 이벤트 타입 판별
            TriggerEventType eventType;
            if (pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
            {
                eventType = TriggerEventType::Enter;
            }
            else if (pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
            {
                eventType = TriggerEventType::Exit;
            }
            else
            {
                continue;
            }

            // 트리거 이벤트 생성
            TriggerEvent event;
            event.type = eventType;
            event.trigger = Ptr<Collider>(trigger);
            event.other = Ptr<Collider>(other);

            // CollisionSystem에 큐잉
            CollisionSystem::Get().QueueTriggerEvent(event);
        }
    }

    void PhysicsEventCallback::onConstraintBreak(
        physx::PxConstraintInfo* constraints,
        physx::PxU32 count)
    {
        // Joint 시스템 구현 시 처리
    }

    void PhysicsEventCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
    {
        // 필요시 구현
    }

    void PhysicsEventCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
    {
        // 필요시 구현
    }

    void PhysicsEventCallback::onAdvance(
        const physx::PxRigidBody* const* bodyBuffer,
        const physx::PxTransform* poseBuffer,
        physx::PxU32 count)
    {
        // CCD 사용 시 구현
    }

    void PhysicsEventCallback::ExtractContactPoints(
        const physx::PxContactPair& pair,
        std::vector<ContactPoint>& outContacts)
    {
        const physx::PxU32 maxContacts = 16;
        physx::PxContactPairPoint contactPoints[maxContacts];
        
        physx::PxU32 nbContacts = pair.extractContacts(contactPoints, maxContacts);

        outContacts.reserve(nbContacts);

        for (physx::PxU32 i = 0; i < nbContacts; ++i)
        {
            const physx::PxContactPairPoint& cp = contactPoints[i];

            ContactPoint point;
            point.point = PhysicsUtility::ToVector3(cp.position);
            point.normal = PhysicsUtility::ToDirection(cp.normal);
            point.separation = cp.separation;
            point.impulse = cp.impulse.magnitude();

            outContacts.push_back(point);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 필터 셰이더
    // ═══════════════════════════════════════════════════════════════

    physx::PxFilterFlags PhysicsFilterShader(
        physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags,
        const void* constantBlock,
        physx::PxU32 constantBlockSize)
    {
        // filterData.word0 = 자신의 레이어 인덱스
        // filterData.word1 = 충돌할 레이어 마스크

        // 트리거 체크
        if (physx::PxFilterObjectIsTrigger(attributes0) || 
            physx::PxFilterObjectIsTrigger(attributes1))
        {
            pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
            return physx::PxFilterFlag::eDEFAULT;
        }

        // 레이어 마스크로 충돌 여부 결정
        // word0: 자신의 레이어 (비트 인덱스)
        // word1: 충돌할 레이어들 (비트 마스크)
        
        physx::PxU32 layer0 = filterData0.word0;
        physx::PxU32 mask0 = filterData0.word1;
        physx::PxU32 layer1 = filterData1.word0;
        physx::PxU32 mask1 = filterData1.word1;

        // 양쪽 모두 상대 레이어와 충돌해야 함
        bool shouldCollide = ((mask0 & (1u << layer1)) != 0) &&
                             ((mask1 & (1u << layer0)) != 0);

        if (!shouldCollide)
        {
            return physx::PxFilterFlag::eSUPPRESS;
        }

        // 충돌 플래그 설정
        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT
                  | physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
                  | physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS
                  | physx::PxPairFlag::eNOTIFY_TOUCH_LOST
                  | physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

        return physx::PxFilterFlag::eDEFAULT;
    }
}
