#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include "Common/Utility/Singleton.h"
#include "Framework/Physics/CollisionTypes.h"

namespace engine
{
    class Collider;
    class GameObject;

    // ═══════════════════════════════════════════════════════════════
    // 공격 인스턴스 (우선순위 시스템용)
    // 하나의 "공격"에 의한 여러 충돌을 그룹화
    // ═══════════════════════════════════════════════════════════════

    class AttackInstance
    {
    private:
        uint64_t m_id;
        Ptr<GameObject> m_attacker;
        std::unordered_set<Handle> m_hitTargets;  // Handle로 추적 (파괴된 오브젝트 안전)
        
        bool m_isConsumed = false;
        CollisionPriority m_consumedBy = CollisionPriority::Default;

    public:
        // 생성자는 cpp에서 정의 (GameObject 완전한 정의 필요)
        AttackInstance(uint64_t id, GameObject* attacker);

        uint64_t GetId() const { return m_id; }
        Ptr<GameObject> GetAttacker() const { return m_attacker; }

        bool HasHit(GameObject* target) const;
        void RecordHit(GameObject* target);

        bool IsConsumed() const { return m_isConsumed; }

        void Consume(CollisionPriority by)
        {
            m_isConsumed = true;
            m_consumedBy = by;
        }

        bool CanProcessWith(CollisionPriority priority) const
        {
            if (!m_isConsumed) return true;
            return static_cast<int32_t>(priority) >= static_cast<int32_t>(m_consumedBy);
        }
    };

    // ═══════════════════════════════════════════════════════════════
    // CollisionSystem - 충돌 이벤트 처리 및 콜백 디스패치
    // ═══════════════════════════════════════════════════════════════

    class CollisionSystem : public Singleton<CollisionSystem>
    {
    private:
        // 이벤트 큐
        std::vector<CollisionEvent> m_pendingCollisionEvents;
        std::vector<TriggerEvent> m_pendingTriggerEvents;

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
                // index와 generation을 조합하여 해시 생성
                size_t h1 = std::hash<uint64_t>()(
                    (static_cast<uint64_t>(p.triggerHandle.index) << 32) | p.triggerHandle.generation);
                size_t h2 = std::hash<uint64_t>()(
                    (static_cast<uint64_t>(p.otherHandle.index) << 32) | p.otherHandle.generation);
                return h1 ^ (h2 << 1);
            }
        };

        std::unordered_set<TriggerPair, TriggerPairHash> m_activeTriggerPairs;

        // 공격 인스턴스 관리 (우선순위 시스템)
        std::unordered_map<uint64_t, AttackInstance> m_activeAttacks;
        uint64_t m_nextAttackId = 1;

    private:
        CollisionSystem() = default;
        ~CollisionSystem() = default;

    public:
        // ═══════════════════════════════════════
        // 이벤트 큐잉 (PhysicsCallback에서 호출)
        // ═══════════════════════════════════════
        void QueueCollisionEvent(const CollisionEvent& event);
        void QueueTriggerEvent(const TriggerEvent& event);

        // ═══════════════════════════════════════
        // 이벤트 처리 (프레임 끝에서 호출)
        // ═══════════════════════════════════════
        void ProcessEvents();

        // ═══════════════════════════════════════
        // 공격 인스턴스 관리 (전투 시스템용)
        // ═══════════════════════════════════════
        uint64_t CreateAttack(GameObject* attacker);
        void EndAttack(uint64_t attackId);
        AttackInstance* GetAttack(uint64_t attackId);

        // ═══════════════════════════════════════
        // Collider 정리 (파괴 시)
        // ═══════════════════════════════════════
        void OnColliderDestroyed(Collider* collider);

        // ═══════════════════════════════════════
        // 이벤트 큐 클리어
        // ═══════════════════════════════════════
        void ClearPendingEvents();

    private:
        void ProcessCollisionEvents();
        void ProcessTriggerEvents();

        void DispatchCollisionEnter(Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts);
        void DispatchCollisionStay(Ptr<Collider> a, Ptr<Collider> b, const std::vector<ContactPoint>& contacts);
        void DispatchCollisionExit(Ptr<Collider> a, Ptr<Collider> b);

        void DispatchTriggerEnter(Ptr<Collider> trigger, Ptr<Collider> other);
        void DispatchTriggerStay(Ptr<Collider> trigger, Ptr<Collider> other);
        void DispatchTriggerExit(Ptr<Collider> trigger, Ptr<Collider> other);

        // 접촉점 노말 반전 (상대방에게 전달할 때)
        std::vector<ContactPoint> FlipContactNormals(const std::vector<ContactPoint>& contacts);

        // Handle로부터 TriggerPair 생성
        TriggerPair MakeTriggerPair(Collider* trigger, Collider* other);

        friend class Singleton<CollisionSystem>;
    };
}
