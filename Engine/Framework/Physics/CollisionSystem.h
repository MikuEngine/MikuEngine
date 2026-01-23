#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include "Framework/Physics/CollisionTypes.h"

namespace engine
{
    class Collider;
    class GameObject;
    class ScriptBase;
    class Scene;

    // ═══════════════════════════════════════════════════════════════
    // CollisionSystem - 충돌 이벤트 처리 및 콜백 디스패치
    // 
    // 설계:
    //   - 이벤트 큐는 Scene이 소유
    //   - CollisionSystem은 이벤트 처리 로직만 담당
    //   - 활성 충돌/트리거 쌍만 자체 관리 (Stay 이벤트 추적용)
    // ═══════════════════════════════════════════════════════════════

    class CollisionSystem
    {
    public:
        CollisionSystem() = default;
        ~CollisionSystem() = default;
        
        // 복사/이동 금지
        CollisionSystem(const CollisionSystem&) = delete;
        CollisionSystem& operator=(const CollisionSystem&) = delete;
        CollisionSystem(CollisionSystem&&) = delete;
        CollisionSystem& operator=(CollisionSystem&&) = delete;

    private:
        // Trigger Stay 추적 (PhysX가 제공하지 않음)
        // Handle 기반으로 추적하여 댕글링 방지
        struct TriggerPair
        {
            Handle triggerHandle;
            Handle otherHandle;

            bool operator==(const TriggerPair& rhs) const
            {
                return triggerHandle == rhs.triggerHandle && otherHandle == rhs.otherHandle;
            }
        };

        struct TriggerPairHash
        {
            size_t operator()(const TriggerPair& p) const
            {
                size_t h1 = std::hash<uint64_t>()(
                    (static_cast<uint64_t>(p.triggerHandle.index) << 32) | p.triggerHandle.generation);
                size_t h2 = std::hash<uint64_t>()(
                    (static_cast<uint64_t>(p.otherHandle.index) << 32) | p.otherHandle.generation);
                return h1 ^ (h2 << 1);
            }
        };

        std::unordered_set<TriggerPair, TriggerPairHash> m_activeTriggerPairs;

        // Collision Stay 추적
        struct CollisionPair
        {
            Handle colliderAHandle;
            Handle colliderBHandle;

            bool operator==(const CollisionPair& rhs) const
            {
                return (colliderAHandle == rhs.colliderAHandle && colliderBHandle == rhs.colliderBHandle) ||
                       (colliderAHandle == rhs.colliderBHandle && colliderBHandle == rhs.colliderAHandle);
            }
        };

        struct CollisionPairHash
        {
            size_t operator()(const CollisionPair& p) const
            {
                size_t h1 = std::hash<uint64_t>()(
                    (static_cast<uint64_t>(p.colliderAHandle.index) << 32) | p.colliderAHandle.generation);
                size_t h2 = std::hash<uint64_t>()(
                    (static_cast<uint64_t>(p.colliderBHandle.index) << 32) | p.colliderBHandle.generation);
                return h1 < h2 ? (h1 ^ (h2 << 1)) : (h2 ^ (h1 << 1));
            }
        };

        std::unordered_set<CollisionPair, CollisionPairHash> m_activeCollisionPairs;

    public:
        // ═══════════════════════════════════════
        // 이벤트 처리 (프레임 끝에서 호출)
        // Scene의 이벤트 큐를 처리
        // ═══════════════════════════════════════
        void ProcessEvents();

        // ═══════════════════════════════════════
        // Collider 정리 (파괴 시)
        // ═══════════════════════════════════════
        void OnColliderDestroyed(Collider* collider);

        // ═══════════════════════════════════════
        // 충돌 상태 조회 (Script에서 사용)
        // ═══════════════════════════════════════
        bool IsColliding(Collider* collider) const;
        std::vector<Collider*> GetCollidingWith(Collider* collider) const;
        bool IsTriggerOverlapping(Collider* trigger) const;
        std::vector<Collider*> GetTriggerOverlaps(Collider* trigger) const;

        // ═══════════════════════════════════════
        // 이벤트 큐 클리어 (하위 호환성)
        // Scene::ClearPendingEvents()로 대체됨
        // ═══════════════════════════════════════
        void ClearPendingEvents();
        
        // ═══════════════════════════════════════
        // 씬 전환 시 활성 쌍 클리어
        // ═══════════════════════════════════════
        void ClearActivePairs();

    private:
        void ProcessCollisionEvents(Scene* scene);
        void ProcessTriggerEvents(Scene* scene);

        void DispatchCollisionEnter(Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts);
        void DispatchCollisionStay(Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts);
        void DispatchCollisionExit(Ptr<Collider> a, Ptr<Collider> b);

        void DispatchTriggerEnter(Ptr<Collider> trigger, Ptr<Collider> other);
        void DispatchTriggerStay(Ptr<Collider> trigger, Ptr<Collider> other);
        void DispatchTriggerExit(Ptr<Collider> trigger, Ptr<Collider> other);

        std::vector<ContactPoint> FlipContactNormals(const std::vector<ContactPoint>& contacts);
        TriggerPair MakeTriggerPair(Collider* trigger, Collider* other);

        void NotifyScriptsCollision(GameObject* go, const CollisionInfo& info, CollisionEventType eventType);
        void NotifyScriptsTrigger(GameObject* go, const CollisionInfo& info, TriggerEventType eventType);
    };
}
