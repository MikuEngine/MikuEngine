#include "EnginePCH.h"
#include "CollisionSystem.h"

#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/Component/Script.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Physics/PhysicsDebugRenderer.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 이벤트 처리
    // ═══════════════════════════════════════════════════════════════

    void CollisionSystem::ProcessEvents()
    {
        Scene* scene = SceneManager::Get().GetScene();
        if (!scene) return;

        // 디버그 렌더러 충돌 상태 초기화
        PhysicsDebugRenderer::Get().ClearCollidingState();

        ProcessCollisionEvents(scene);
        ProcessTriggerEvents(scene);

        // 활성 충돌 쌍에 대해 매 프레임 MarkColliding 호출
        for (const auto& pair : m_activeCollisionPairs)
        {
            Ptr<Collider> colliderA(pair.colliderAHandle);
            Ptr<Collider> colliderB(pair.colliderBHandle);
            
            if (colliderA && colliderB)
            {
                PhysicsDebugRenderer::Get().MarkColliding(colliderA.Get());
                PhysicsDebugRenderer::Get().MarkColliding(colliderB.Get());
            }
        }

        // 활성 트리거 쌍에 대해서도 MarkColliding 호출
        for (const auto& pair : m_activeTriggerPairs)
        {
            Ptr<Collider> trigger(pair.triggerHandle);
            Ptr<Collider> other(pair.otherHandle);
            
            if (trigger && other)
            {
                PhysicsDebugRenderer::Get().MarkColliding(trigger.Get());
                PhysicsDebugRenderer::Get().MarkColliding(other.Get());
            }
        }
    }

    void CollisionSystem::ProcessCollisionEvents(Scene* scene)
    {
        auto& pendingEvents = scene->GetPendingCollisionEvents();
        
        if (pendingEvents.empty())
        {
            return;
        }

        // 우선순위로 정렬 (높은 것 먼저)
        std::sort(pendingEvents.begin(), pendingEvents.end(),
            [](const CollisionEvent& a, const CollisionEvent& b)
            {
                return static_cast<int32_t>(a.priority) > static_cast<int32_t>(b.priority);
            });

        // 순서대로 처리
        for (CollisionEvent& event : pendingEvents)
        {
            // Ptr 유효성 검사
            if (!event.colliderA || !event.colliderB)
            {
                continue;
            }

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
        }

        pendingEvents.clear();
    }

    void CollisionSystem::ProcessTriggerEvents(Scene* scene)
    {
        auto& pendingEvents = scene->GetPendingTriggerEvents();
        
        // Enter/Exit 이벤트 처리
        for (const TriggerEvent& event : pendingEvents)
        {
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

        pendingEvents.clear();

        // Stay 이벤트 발생 (매 프레임 활성 쌍에 대해)
        std::vector<TriggerPair> invalidPairs;

        for (const TriggerPair& pair : m_activeTriggerPairs)
        {
            Ptr<Collider> trigger(pair.triggerHandle);
            Ptr<Collider> other(pair.otherHandle);

            if (trigger && other)
            {
                DispatchTriggerStay(trigger, other);
            }
            else
            {
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
    }

    void CollisionSystem::ClearPendingEvents()
    {
        Scene* scene = SceneManager::Get().GetScene();
        if (scene)
        {
            scene->ClearPendingEvents();
        }
    }

    void CollisionSystem::ClearActivePairs()
    {
        m_activeTriggerPairs.clear();
        m_activeCollisionPairs.clear();
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
            if (ScriptBase* script = comp->As<ScriptBase>())
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
            if (ScriptBase* script = comp->As<ScriptBase>())
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
        if (!a || !b) return;

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

        NotifyScriptsCollision(goA, infoForA, CollisionEventType::Enter);
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
        if (!a || !b) return;

        PhysicsDebugRenderer::Get().MarkColliding(a.Get());
        PhysicsDebugRenderer::Get().MarkColliding(b.Get());

        GameObject* goA = a->GetGameObject();
        GameObject* goB = b->GetGameObject();

        if (!goA || !goB) return;

        CollisionInfo infoForA;
        infoForA.collider = b;
        infoForA.rigidbody = Ptr<Rigidbody>(b->GetAttachedRigidbody());
        infoForA.gameObject = Ptr<GameObject>(goB);
        infoForA.contacts = contacts;

        CollisionInfo infoForB;
        infoForB.collider = a;
        infoForB.rigidbody = Ptr<Rigidbody>(a->GetAttachedRigidbody());
        infoForB.gameObject = Ptr<GameObject>(goA);
        infoForB.contacts = FlipContactNormals(contacts);

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

        PhysicsDebugRenderer::Get().MarkColliding(trigger.Get());
        PhysicsDebugRenderer::Get().MarkColliding(other.Get());

        GameObject* goTrigger = trigger->GetGameObject();
        GameObject* goOther = other->GetGameObject();

        if (!goTrigger || !goOther) return;

        CollisionInfo infoForTrigger;
        infoForTrigger.collider = other;
        infoForTrigger.rigidbody = Ptr<Rigidbody>(other->GetAttachedRigidbody());
        infoForTrigger.gameObject = Ptr<GameObject>(goOther);

        CollisionInfo infoForOther;
        infoForOther.collider = trigger;
        infoForOther.rigidbody = Ptr<Rigidbody>(trigger->GetAttachedRigidbody());
        infoForOther.gameObject = Ptr<GameObject>(goTrigger);

        NotifyScriptsTrigger(goTrigger, infoForTrigger, TriggerEventType::Enter);
        NotifyScriptsTrigger(goOther, infoForOther, TriggerEventType::Enter);
    }

    void CollisionSystem::DispatchTriggerStay(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;

        PhysicsDebugRenderer::Get().MarkColliding(trigger.Get());
        PhysicsDebugRenderer::Get().MarkColliding(other.Get());

        GameObject* goTrigger = trigger->GetGameObject();
        GameObject* goOther = other->GetGameObject();

        if (!goTrigger || !goOther) return;

        CollisionInfo infoForTrigger;
        infoForTrigger.collider = other;
        infoForTrigger.rigidbody = Ptr<Rigidbody>(other->GetAttachedRigidbody());
        infoForTrigger.gameObject = Ptr<GameObject>(goOther);

        CollisionInfo infoForOther;
        infoForOther.collider = trigger;
        infoForOther.rigidbody = Ptr<Rigidbody>(trigger->GetAttachedRigidbody());
        infoForOther.gameObject = Ptr<GameObject>(goTrigger);

        NotifyScriptsTrigger(goTrigger, infoForTrigger, TriggerEventType::Stay);
        NotifyScriptsTrigger(goOther, infoForOther, TriggerEventType::Stay);
    }

    void CollisionSystem::DispatchTriggerExit(Ptr<Collider> trigger, Ptr<Collider> other)
    {
        if (!trigger || !other) return;

        GameObject* goTrigger = trigger->GetGameObject();
        GameObject* goOther = other->GetGameObject();

        if (!goTrigger || !goOther) return;

        CollisionInfo infoForTrigger;
        infoForTrigger.collider = other;
        infoForTrigger.rigidbody = Ptr<Rigidbody>(other->GetAttachedRigidbody());
        infoForTrigger.gameObject = Ptr<GameObject>(goOther);

        CollisionInfo infoForOther;
        infoForOther.collider = trigger;
        infoForOther.rigidbody = Ptr<Rigidbody>(trigger->GetAttachedRigidbody());
        infoForOther.gameObject = Ptr<GameObject>(goTrigger);

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
                Ptr<Collider> other(pair.colliderBHandle);
                if (other)
                {
                    result.push_back(other.Get());
                }
            }
            else if (pair.colliderBHandle == handle)
            {
                Ptr<Collider> other(pair.colliderAHandle);
                if (other)
                {
                    result.push_back(other.Get());
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
                Ptr<Collider> other(pair.otherHandle);
                if (other)
                {
                    result.push_back(other.Get());
                }
            }
            else if (pair.otherHandle == handle)
            {
                Ptr<Collider> other(pair.triggerHandle);
                if (other)
                {
                    result.push_back(other.Get());
                }
            }
        }
        return result;
    }
}
