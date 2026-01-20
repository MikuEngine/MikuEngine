#include "EnginePCH.h"
#include "PhysicsCallback.h"

#include "Framework/Physics/CollisionSystem.h"
#include "Framework/Physics/PhysicsLayer.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/GameObject/GameObject.h"

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
        LOG_PRINT("[PhysicsCallback] onContact called with {} pairs", nbPairs);
        
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

            // Shape nullptr 체크
            if (!pair.shapes[0] || !pair.shapes[1])
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
                LOG_PRINT("[PhysicsCallback] Unknown event flags: 0x{:X}", static_cast<unsigned int>(pair.events));
                continue;
            }
            
            LOG_PRINT("[PhysicsCallback] Event type determined: {} (flags=0x{:X})", 
                static_cast<int>(eventType), static_cast<unsigned int>(pair.events));

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
            LOG_PRINT("[PhysicsCallback] Queuing collision event: {} <-> {}, type={}",
                colliderA->GetGameObject()->GetName(),
                colliderB->GetGameObject()->GetName(),
                static_cast<int>(eventType));
            CollisionSystem::Get().QueueCollisionEvent(event);
        }
    }

    void PhysicsEventCallback::onTrigger(
        physx::PxTriggerPair* pairs,
        physx::PxU32 count)
    {
        LOG_PRINT("[PhysicsCallback] onTrigger called with {} pairs", count);
        
        for (physx::PxU32 i = 0; i < count; ++i)
        {
            const physx::PxTriggerPair& pair = pairs[i];

            // 삭제된 Shape 스킵
            if (pair.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER |
                              physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
            {
                continue;
            }

            // Shape nullptr 체크
            if (!pair.triggerShape || !pair.otherShape)
            {
                continue;
            }

            // Actor nullptr 체크
            if (!pair.triggerActor || !pair.otherActor)
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
        // Kinematic 확인 (먼저 체크)
        bool isKinematic0 = physx::PxFilterObjectIsKinematic(attributes0);
        bool isKinematic1 = physx::PxFilterObjectIsKinematic(attributes1);
        
        // 필터 셰이더 호출 확인 (Kinematic 쌍은 항상 로그)
        static int totalCalls = 0;
        static int kinematicPairCalls = 0;
        totalCalls++;
        
        // Kinematic 쌍은 항상 로그
        if (isKinematic0 || isKinematic1)
        {
            kinematicPairCalls++;
            LOG_PRINT("[PhysicsFilterShader] Called #{} (Kinematic pair #{}): layer0={}, mask0=0x{:X}, layer1={}, mask1=0x{:X}, K0={}, K1={}",
                totalCalls, kinematicPairCalls, filterData0.word0, filterData0.word1, filterData1.word0, filterData1.word1,
                isKinematic0, isKinematic1);
        }
        else if (totalCalls <= 10)  // 처음 10번은 로그
        {
            LOG_PRINT("[PhysicsFilterShader] Called #{}: layer0={}, mask0=0x{:X}, layer1={}, mask1=0x{:X}",
                totalCalls, filterData0.word0, filterData0.word1, filterData1.word0, filterData1.word1);
        }

        // filterData.word0 = 자신의 레이어 인덱스
        // filterData.word1 = 충돌할 레이어 마스크

        // ═══════════════════════════════════════════════════════════════
        // 레이어 마스크 체크 (트리거/일반 충돌 모두에 적용)
        // ═══════════════════════════════════════════════════════════════
        physx::PxU32 layer0 = filterData0.word0;
        physx::PxU32 mask0 = filterData0.word1;
        physx::PxU32 layer1 = filterData1.word0;
        physx::PxU32 mask1 = filterData1.word1;

        // 양쪽 모두 상대 레이어와 충돌해야 함
        bool shouldCollide = ((mask0 & (1u << layer1)) != 0) &&
                             ((mask1 & (1u << layer0)) != 0);

        // 레이어 마스크로 충돌하지 않으면 즉시 무시 (트리거 포함)
        if (!shouldCollide)
        {
            return physx::PxFilterFlag::eSUPPRESS;
        }

        // ═══════════════════════════════════════════════════════════════
        // 트리거 체크 (레이어 체크 통과 후)
        // ═══════════════════════════════════════════════════════════════
        if (physx::PxFilterObjectIsTrigger(attributes0) || 
            physx::PxFilterObjectIsTrigger(attributes1))
        {
            pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
            return physx::PxFilterFlag::eDEFAULT;
        }

        // ═══════════════════════════════════════════════════════════════
        // 모든 충돌 쌍에 대해 기본 처리
        // - Kinematic-Dynamic: PhysX가 Dynamic을 밀어냄 (물리 반응)
        // - Kinematic-Static: PhysX가 기본적으로 무시 (여기까지 도달 안 함)
        // - Dynamic-Dynamic: 서로 밀어냄 (물리 반응)
        // - Dynamic-Static: Dynamic이 밀림 (물리 반응)
        // ═══════════════════════════════════════════════════════════════
        
        // 모든 쌍에 대해 물리 반응 + 이벤트 알림
        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT
                  | physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
                  | physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS
                  | physx::PxPairFlag::eNOTIFY_TOUCH_LOST
                  | physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

        return physx::PxFilterFlag::eDEFAULT;
    }
}
