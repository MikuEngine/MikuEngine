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
    }

    void CollisionSystem::ProcessCollisionEvents()
    {
        if (m_pendingCollisionEvents.empty())
        {
            return;
        }

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
    // 콜백 디스패치
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::DispatchCollisionEnter(
        Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts)
    {
        // 디스패치 시점에 다시 유효성 검사
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

        // A의 모든 Script에 알림
        // TODO: Script 시스템과 연동 필요
        // 현재는 주석 처리된 OnCollisionEnter가 활성화되면 호출

        // B의 모든 Script에 알림
    }

    void CollisionSystem::DispatchCollisionStay(
        Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts)
    {
        // Enter와 유사하게 구현
        if (!a || !b) return;

        // 디버그 렌더러에 충돌 상태 표시
        PhysicsDebugRenderer::Get().MarkColliding(a.Get());
        PhysicsDebugRenderer::Get().MarkColliding(b.Get());

        // TODO: Script 콜백 구현
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
    }

    void CollisionSystem::DispatchTriggerEnter(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;

        // Trigger 측 Script들에 알림
        // Other 측 Script들에도 알림
    }

    void CollisionSystem::DispatchTriggerStay(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;
        // TODO: 구현
    }

    void CollisionSystem::DispatchTriggerExit(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;
        // TODO: 구현
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

    CollisionSystem::TriggerPair CollisionSystem::MakeTriggerPair(Collider* trigger, Collider* other)
    {
        TriggerPair pair;
        if (trigger) pair.triggerHandle = trigger->GetHandle();
        if (other) pair.otherHandle = other->GetHandle();
        return pair;
    }
}
