#include "EnginePCH.h"
#include "CollisionSystem.h"

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/Component/Script.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Physics/PhysicsDebugRenderer.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // AttackInstance 구현
    // ═══════════════════════════════════════════════════════════════

    AttackInstance::AttackInstance(uint64_t id, GameObject* attacker)
        : m_id(id)
        , m_attacker(attacker)  // Ptr<GameObject>(GameObject*) - GameObject.h 필요
    {
    }

    bool AttackInstance::HasHit(GameObject* target) const
    {
        if (!target) return false;
        return m_hitTargets.find(target->GetHandle()) != m_hitTargets.end();
    }

    void AttackInstance::RecordHit(GameObject* target)
    {
        if (target)
        {
            m_hitTargets.insert(target->GetHandle());
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 이벤트 큐잉
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::QueueCollisionEvent(const CollisionEvent& event)
    {
        m_pendingCollisionEvents.push_back(event);
    }

    void CollisionSystem::QueueTriggerEvent(const TriggerEvent& event)
    {
        m_pendingTriggerEvents.push_back(event);
    }

    // ═══════════════════════════════════════════════════════════════
    // 이벤트 처리
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::ProcessEvents()
    {
        // 디버그 렌더러 충돌 상태 초기화
        PhysicsDebugRenderer::Get().ClearCollidingState();

        ProcessCollisionEvents();
        ProcessTriggerEvents();

        // 활성 충돌 쌍에 대해 매 프레임 MarkColliding 호출 (Stay 이벤트가 없어도 시각적 피드백 유지)
        for (const auto& pair : m_activeCollisionPairs)
        {
            Object* objA = GetObjectFromHandle(pair.colliderAHandle);
            Object* objB = GetObjectFromHandle(pair.colliderBHandle);
            
            Collider* colliderA = dynamic_cast<Collider*>(objA);
            Collider* colliderB = dynamic_cast<Collider*>(objB);
            
            if (colliderA && colliderB)
            {
                PhysicsDebugRenderer::Get().MarkColliding(colliderA);
                PhysicsDebugRenderer::Get().MarkColliding(colliderB);
            }
        }

        // 활성 트리거 쌍에 대해서도 MarkColliding 호출
        for (const auto& pair : m_activeTriggerPairs)
        {
            Object* objTrigger = GetObjectFromHandle(pair.triggerHandle);
            Object* objOther = GetObjectFromHandle(pair.otherHandle);
            
            Collider* trigger = dynamic_cast<Collider*>(objTrigger);
            Collider* other = dynamic_cast<Collider*>(objOther);
            
            if (trigger && other)
            {
                PhysicsDebugRenderer::Get().MarkColliding(trigger);
                PhysicsDebugRenderer::Get().MarkColliding(other);
            }
        }
    }

    void CollisionSystem::ProcessCollisionEvents()
    {
        if (m_pendingCollisionEvents.empty())
        {
            return;
        }

        LOG_PRINT("[CollisionSystem] Processing {} collision events", m_pendingCollisionEvents.size());

        // 우선순위로 정렬 (높은 것 먼저)
        std::sort(m_pendingCollisionEvents.begin(), m_pendingCollisionEvents.end(),
            [](const CollisionEvent& a, const CollisionEvent& b)
            {
                return static_cast<int32_t>(a.priority) > static_cast<int32_t>(b.priority);
            });

        // 순서대로 처리
        for (CollisionEvent& event : m_pendingCollisionEvents)
        {
            // Ptr 유효성 검사 - 파괴된 오브젝트는 건너뜀
            if (!event.colliderA || !event.colliderB)
            {
                continue;
            }

            // 우선순위 시스템: 공격 인스턴스 체크
            if (event.sourceId != 0)
            {
                AttackInstance* attack = GetAttack(event.sourceId);
                if (attack)
                {
                    // 이미 더 높은 우선순위에 의해 소비됨?
                    if (!attack->CanProcessWith(event.priority))
                    {
                        continue;
                    }

                    // 이 대상을 이미 처리함?
                    GameObject* targetA = event.colliderA->GetGameObject();
                    GameObject* targetB = event.colliderB->GetGameObject();
                    
                    if (attack->HasHit(targetA) || attack->HasHit(targetB))
                    {
                        continue;
                    }
                }
            }

            // 이벤트 타입에 따라 디스패치
            LOG_PRINT("[CollisionSystem] Dispatching collision event: type={}, colliderA={}, colliderB={}",
                static_cast<int>(event.type),
                event.colliderA ? event.colliderA->GetGameObject()->GetName() : "null",
                event.colliderB ? event.colliderB->GetGameObject()->GetName() : "null");
            
            switch (event.type)
            {
            case CollisionEventType::Enter:
                DispatchCollisionEnter(event.colliderA, event.colliderB, event.contacts);
                break;

            case CollisionEventType::Stay:
                DispatchCollisionStay(event.colliderA, event.colliderB, event.contacts);
                break;

            case CollisionEventType::Exit:
                DispatchCollisionExit(event.colliderA, event.colliderB);
                break;
            }

            // 공격 인스턴스에 히트 기록
            if (event.sourceId != 0)
            {
                AttackInstance* attack = GetAttack(event.sourceId);
                if (attack)
                {
                    // 콜백 후 다시 유효성 검사 (콜백에서 파괴됐을 수 있음)
                    if (event.colliderA)
                    {
                        attack->RecordHit(event.colliderA->GetGameObject());
                    }
                    if (event.colliderB)
                    {
                        attack->RecordHit(event.colliderB->GetGameObject());
                    }

                    // 높은 우선순위(패링 등)면 공격 소비
                    if (event.priority >= CollisionPriority::Block)
                    {
                        attack->Consume(event.priority);
                    }
                }
            }
        }

        m_pendingCollisionEvents.clear();
    }

    void CollisionSystem::ProcessTriggerEvents()
    {
        if (!m_pendingTriggerEvents.empty())
        {
            LOG_PRINT("[CollisionSystem] Processing {} trigger events", m_pendingTriggerEvents.size());
        }
        
        // Enter/Exit 이벤트 처리
        for (const TriggerEvent& event : m_pendingTriggerEvents)
        {
            // Ptr 유효성 검사
            if (!event.trigger || !event.other)
            {
                continue;
            }

            TriggerPair pair = MakeTriggerPair(event.trigger.Get(), event.other.Get());

            if (event.type == TriggerEventType::Enter)
            {
                m_activeTriggerPairs.insert(pair);
                DispatchTriggerEnter(event.trigger, event.other);
            }
            else if (event.type == TriggerEventType::Exit)
            {
                m_activeTriggerPairs.erase(pair);
                DispatchTriggerExit(event.trigger, event.other);
            }
        }

        m_pendingTriggerEvents.clear();

        // Stay 이벤트 발생 (매 프레임 활성 쌍에 대해)
        // 유효하지 않은 쌍은 제거
        std::vector<TriggerPair> invalidPairs;

        for (const TriggerPair& pair : m_activeTriggerPairs)
        {
            // Handle로부터 Collider 복원 (Ptr 사용)
            Ptr<Collider> trigger(pair.triggerHandle);
            Ptr<Collider> other(pair.otherHandle);

            if (trigger && other)
            {
                DispatchTriggerStay(trigger, other);
            }
            else
            {
                // 파괴된 오브젝트가 있으면 나중에 제거
                invalidPairs.push_back(pair);
            }
        }

        // 유효하지 않은 쌍 제거
        for (const TriggerPair& pair : invalidPairs)
        {
            m_activeTriggerPairs.erase(pair);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 인스턴스 관리
    // ═══════════════════════════════════════════════════════════════

    uint64_t CollisionSystem::CreateAttack(GameObject* attacker)
    {
        uint64_t id = m_nextAttackId++;
        m_activeAttacks.emplace(id, AttackInstance(id, attacker));
        return id;
    }

    void CollisionSystem::EndAttack(uint64_t attackId)
    {
        m_activeAttacks.erase(attackId);
    }

    AttackInstance* CollisionSystem::GetAttack(uint64_t attackId)
    {
        auto it = m_activeAttacks.find(attackId);
        if (it != m_activeAttacks.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════════════
    // Collider 정리
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::OnColliderDestroyed(Collider* collider)
    {
        if (!collider) return;

        Handle colliderHandle = collider->GetHandle();

        // Trigger 쌍에서 제거
        for (auto it = m_activeTriggerPairs.begin(); it != m_activeTriggerPairs.end(); )
        {
            if (it->triggerHandle == colliderHandle || it->otherHandle == colliderHandle)
            {
                it = m_activeTriggerPairs.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 활성 충돌 쌍에서 제거
        for (auto it = m_activeCollisionPairs.begin(); it != m_activeCollisionPairs.end(); )
        {
            if (it->colliderAHandle == colliderHandle || it->colliderBHandle == colliderHandle)
            {
                it = m_activeCollisionPairs.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 대기 중인 이벤트에서 제거
        // Ptr은 자동으로 무효화되므로 명시적 제거는 선택사항이지만,
        // 메모리 절약을 위해 미리 제거
        m_pendingCollisionEvents.erase(
            std::remove_if(m_pendingCollisionEvents.begin(), m_pendingCollisionEvents.end(),
                [collider](const CollisionEvent& e)
                {
                    return e.colliderA.Get() == collider || e.colliderB.Get() == collider;
                }),
            m_pendingCollisionEvents.end()
        );

        m_pendingTriggerEvents.erase(
            std::remove_if(m_pendingTriggerEvents.begin(), m_pendingTriggerEvents.end(),
                [collider](const TriggerEvent& e)
                {
                    return e.trigger.Get() == collider || e.other.Get() == collider;
                }),
            m_pendingTriggerEvents.end()
        );
    }

    void CollisionSystem::ClearPendingEvents()
    {
        m_pendingCollisionEvents.clear();
        m_pendingTriggerEvents.clear();
    }

    // ═══════════════════════════════════════════════════════════════
    // Script 콜백 헬퍼
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::NotifyScriptsCollision(
        GameObject* go, const CollisionInfo& info, CollisionEventType eventType)
    {
        if (!go) return;

        const auto& components = go->GetComponents();
        for (const auto& comp : components)
        {
            if (ScriptBase* script = dynamic_cast<ScriptBase*>(comp.get()))
            {
                if (!script->IsActive()) continue;

                switch (eventType)
                {
                case CollisionEventType::Enter:
                    script->OnCollisionEnter(info);
                    break;
                case CollisionEventType::Stay:
                    script->OnCollisionStay(info);
                    break;
                case CollisionEventType::Exit:
                    script->OnCollisionExit(info);
                    break;
                }
            }
        }
    }

    void CollisionSystem::NotifyScriptsTrigger(
        GameObject* go, const CollisionInfo& info, TriggerEventType eventType)
    {
        if (!go) return;

        const auto& components = go->GetComponents();
        for (const auto& comp : components)
        {
            if (ScriptBase* script = dynamic_cast<ScriptBase*>(comp.get()))
            {
                if (!script->IsActive()) continue;

                switch (eventType)
                {
                case TriggerEventType::Enter:
                    script->OnTriggerEnter(info);
                    break;
                case TriggerEventType::Stay:
                    script->OnTriggerStay(info);
                    break;
                case TriggerEventType::Exit:
                    script->OnTriggerExit(info);
                    break;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 콜백 디스패치
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::DispatchCollisionEnter(
        Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts)
    {
        // 디스패치 시점에 다시 유효성 검사
        if (!a || !b) return;

        // 디버그 렌더러에 충돌 상태 표시
        LOG_PRINT("[CollisionSystem] MarkColliding: {} <-> {}", 
            a->GetGameObject()->GetName(), 
            b->GetGameObject()->GetName());
        PhysicsDebugRenderer::Get().MarkColliding(a.Get());
        PhysicsDebugRenderer::Get().MarkColliding(b.Get());

        GameObject* goA = a->GetGameObject();
        GameObject* goB = b->GetGameObject();

        if (!goA || !goB) return;

        // A에게 전달할 정보
        CollisionInfo infoForA;
        infoForA.collider = b;
        infoForA.rigidbody = Ptr<Rigidbody>(b->GetAttachedRigidbody());
        infoForA.gameObject = Ptr<GameObject>(goB);
        infoForA.contacts = contacts;

        // B에게 전달할 정보 (노말 반전)
        CollisionInfo infoForB;
        infoForB.collider = a;
        infoForB.rigidbody = Ptr<Rigidbody>(a->GetAttachedRigidbody());
        infoForB.gameObject = Ptr<GameObject>(goA);
        infoForB.contacts = FlipContactNormals(contacts);

        // A의 모든 Script에 알림
        NotifyScriptsCollision(goA, infoForA, CollisionEventType::Enter);

        // B의 모든 Script에도 알림
        NotifyScriptsCollision(goB, infoForB, CollisionEventType::Enter);

        // 활성 충돌 쌍에 추가
        CollisionPair pair;
        pair.colliderAHandle = a.GetHandle();
        pair.colliderBHandle = b.GetHandle();
        m_activeCollisionPairs.insert(pair);
    }

    void CollisionSystem::DispatchCollisionStay(
        Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts)
    {
        // Enter와 유사하게 구현
        if (!a || !b) return;

        // 디버그 렌더러에 충돌 상태 표시
        PhysicsDebugRenderer::Get().MarkColliding(a.Get());
        PhysicsDebugRenderer::Get().MarkColliding(b.Get());

        GameObject* goA = a->GetGameObject();
        GameObject* goB = b->GetGameObject();

        if (!goA || !goB) return;

        // A에게 전달할 정보
        CollisionInfo infoForA;
        infoForA.collider = b;
        infoForA.rigidbody = Ptr<Rigidbody>(b->GetAttachedRigidbody());
        infoForA.gameObject = Ptr<GameObject>(goB);
        infoForA.contacts = contacts;

        // B에게 전달할 정보 (노말 반전)
        CollisionInfo infoForB;
        infoForB.collider = a;
        infoForB.rigidbody = Ptr<Rigidbody>(a->GetAttachedRigidbody());
        infoForB.gameObject = Ptr<GameObject>(goA);
        infoForB.contacts = FlipContactNormals(contacts);

        // Script에 알림
        NotifyScriptsCollision(goA, infoForA, CollisionEventType::Stay);
        NotifyScriptsCollision(goB, infoForB, CollisionEventType::Stay);
    }

    void CollisionSystem::DispatchCollisionExit(Ptr<Collider> a, Ptr<Collider> b)
    {
        if (!a || !b) return;

        GameObject* goA = a->GetGameObject();
        GameObject* goB = b->GetGameObject();

        if (!goA || !goB) return;

        CollisionInfo infoForA;
        infoForA.collider = b;
        infoForA.rigidbody = Ptr<Rigidbody>(b->GetAttachedRigidbody());
        infoForA.gameObject = Ptr<GameObject>(goB);

        CollisionInfo infoForB;
        infoForB.collider = a;
        infoForB.rigidbody = Ptr<Rigidbody>(a->GetAttachedRigidbody());
        infoForB.gameObject = Ptr<GameObject>(goA);

        // Script 콜백 호출
        NotifyScriptsCollision(goA, infoForA, CollisionEventType::Exit);
        NotifyScriptsCollision(goB, infoForB, CollisionEventType::Exit);

        // 활성 충돌 쌍에서 제거
        CollisionPair pair;
        pair.colliderAHandle = a.GetHandle();
        pair.colliderBHandle = b.GetHandle();
        m_activeCollisionPairs.erase(pair);
    }

    void CollisionSystem::DispatchTriggerEnter(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;

        // 디버그 렌더러에 충돌 상태 표시
        LOG_PRINT("[CollisionSystem] TriggerEnter: {} <-> {}", 
            trigger->GetGameObject()->GetName(),
            other->GetGameObject()->GetName());
        PhysicsDebugRenderer::Get().MarkColliding(trigger.Get());
        PhysicsDebugRenderer::Get().MarkColliding(other.Get());

        GameObject* goTrigger = trigger->GetGameObject();
        GameObject* goOther = other->GetGameObject();

        if (!goTrigger || !goOther) return;

        // Trigger 측에 전달할 정보
        CollisionInfo infoForTrigger;
        infoForTrigger.collider = other;
        infoForTrigger.rigidbody = Ptr<Rigidbody>(other->GetAttachedRigidbody());
        infoForTrigger.gameObject = Ptr<GameObject>(goOther);

        // Other 측에 전달할 정보
        CollisionInfo infoForOther;
        infoForOther.collider = trigger;
        infoForOther.rigidbody = Ptr<Rigidbody>(trigger->GetAttachedRigidbody());
        infoForOther.gameObject = Ptr<GameObject>(goTrigger);

        // Script에 알림
        NotifyScriptsTrigger(goTrigger, infoForTrigger, TriggerEventType::Enter);
        NotifyScriptsTrigger(goOther, infoForOther, TriggerEventType::Enter);
    }

    void CollisionSystem::DispatchTriggerStay(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;

        // 디버그 렌더러에 충돌 상태 표시
        PhysicsDebugRenderer::Get().MarkColliding(trigger.Get());
        PhysicsDebugRenderer::Get().MarkColliding(other.Get());

        GameObject* goTrigger = trigger->GetGameObject();
        GameObject* goOther = other->GetGameObject();

        if (!goTrigger || !goOther) return;

        // Trigger 측에 전달할 정보
        CollisionInfo infoForTrigger;
        infoForTrigger.collider = other;
        infoForTrigger.rigidbody = Ptr<Rigidbody>(other->GetAttachedRigidbody());
        infoForTrigger.gameObject = Ptr<GameObject>(goOther);

        // Other 측에 전달할 정보
        CollisionInfo infoForOther;
        infoForOther.collider = trigger;
        infoForOther.rigidbody = Ptr<Rigidbody>(trigger->GetAttachedRigidbody());
        infoForOther.gameObject = Ptr<GameObject>(goTrigger);

        // Script에 알림
        NotifyScriptsTrigger(goTrigger, infoForTrigger, TriggerEventType::Stay);
        NotifyScriptsTrigger(goOther, infoForOther, TriggerEventType::Stay);
    }

    void CollisionSystem::DispatchTriggerExit(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;

        GameObject* goTrigger = trigger->GetGameObject();
        GameObject* goOther = other->GetGameObject();

        if (!goTrigger || !goOther) return;

        // Trigger 측에 전달할 정보
        CollisionInfo infoForTrigger;
        infoForTrigger.collider = other;
        infoForTrigger.rigidbody = Ptr<Rigidbody>(other->GetAttachedRigidbody());
        infoForTrigger.gameObject = Ptr<GameObject>(goOther);

        // Other 측에 전달할 정보
        CollisionInfo infoForOther;
        infoForOther.collider = trigger;
        infoForOther.rigidbody = Ptr<Rigidbody>(trigger->GetAttachedRigidbody());
        infoForOther.gameObject = Ptr<GameObject>(goTrigger);

        // Script에 알림
        NotifyScriptsTrigger(goTrigger, infoForTrigger, TriggerEventType::Exit);
        NotifyScriptsTrigger(goOther, infoForOther, TriggerEventType::Exit);
    }

    std::vector<ContactPoint> CollisionSystem::FlipContactNormals(
        const std::vector<ContactPoint>& contacts)
    {
        std::vector<ContactPoint> flipped = contacts;
        for (ContactPoint& cp : flipped)
        {
            cp.normal = -cp.normal;
        }
        return flipped;
    }

    //TriggerPair구조체 내부에서 Handle 멤버 사용함. 
    CollisionSystem::TriggerPair CollisionSystem::MakeTriggerPair(Collider* trigger, Collider* other)
    {
        TriggerPair pair;
        if (trigger) pair.triggerHandle = trigger->GetHandle();
        if (other) pair.otherHandle = other->GetHandle();
        return pair;
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 상태 조회
    // ═══════════════════════════════════════════════════════════════

    bool CollisionSystem::IsColliding(Collider* collider) const
    {
        if (!collider) return false;
        
        Handle handle = collider->GetHandle();
        for (const auto& pair : m_activeCollisionPairs)
        {
            if (pair.colliderAHandle == handle || pair.colliderBHandle == handle)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<Collider*> CollisionSystem::GetCollidingWith(Collider* collider) const
    {
        std::vector<Collider*> result;
        if (!collider) return result;
        
        Handle handle = collider->GetHandle();
        for (const auto& pair : m_activeCollisionPairs)
        {
            if (pair.colliderAHandle == handle)
            {
                // colliderB 찾기
                if (Object* obj = GetObjectFromHandle(pair.colliderBHandle))
                {
                    if (Collider* other = dynamic_cast<Collider*>(obj))
                    {
                        result.push_back(other);
                    }
                }
            }
            else if (pair.colliderBHandle == handle)
            {
                // colliderA 찾기
                if (Object* obj = GetObjectFromHandle(pair.colliderAHandle))
                {
                    if (Collider* other = dynamic_cast<Collider*>(obj))
                    {
                        result.push_back(other);
                    }
                }
            }
        }
        return result;
    }

    bool CollisionSystem::IsTriggerOverlapping(Collider* trigger) const
    {
        if (!trigger) return false;
        
        Handle handle = trigger->GetHandle();
        for (const auto& pair : m_activeTriggerPairs)
        {
            if (pair.triggerHandle == handle || pair.otherHandle == handle)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<Collider*> CollisionSystem::GetTriggerOverlaps(Collider* trigger) const
    {
        std::vector<Collider*> result;
        if (!trigger) return result;
        
        Handle handle = trigger->GetHandle();
        for (const auto& pair : m_activeTriggerPairs)
        {
            if (pair.triggerHandle == handle)
            {
                if (Object* obj = GetObjectFromHandle(pair.otherHandle))
                {
                    if (Collider* other = dynamic_cast<Collider*>(obj))
                    {
                        result.push_back(other);
                    }
                }
            }
            else if (pair.otherHandle == handle)
            {
                if (Object* obj = GetObjectFromHandle(pair.triggerHandle))
                {
                    if (Collider* other = dynamic_cast<Collider*>(obj))
                    {
                        result.push_back(other);
                    }
                }
            }
        }
        return result;
    }
}
