#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class StaticMeshRenderer;
}

namespace game
{
    class BossScript;
    class PlayerControllerScript;

    class BossProjectile :
        public engine::Script<BossProjectile>
    {
        REGISTER_SCRIPT(BossProjectile, Script)

    private:
        engine::Vector3 m_direction;  // 이동 방향 (정규화됨)
        float m_speed = 3.0f;  // 이동 속도
        float m_damage = 30.0f;  // 데미지
        float m_lifetime = 10.0f;  // 수명 (초)
        float m_elapsedTime = 0.0f;  // 경과 시간

        bool m_isCrystallized = false;  // 결정화 상태
        bool m_canBeCrystallized = true;  // 결정화 가능 여부
        bool m_isReflecting = false;
        engine::Ptr<BossScript> m_boss;  // 보스 참조 (반사 시 필요)
        std::vector<engine::Ptr<engine::GameObject>> m_pillarCrystalizedPieces;

        bool m_isDestroyed = false;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        void Setup(const engine::Vector3& direction, float speed, float damage, float lifetime, BossScript* boss);

        // ─────────────────────────────────────────────
        // 결정화/처형 시스템
        // ─────────────────────────────────────────────
        void OnCrystallized();  // 결정화 처리
        void OnExecutionReflected(const engine::Vector3& direction);  // 처형 반사 처리
        void Execute();  // 처형 처리 (플레이어->구체 방향으로 반사)

        // ─────────────────────────────────────────────
        // 충돌 처리
        // ─────────────────────────────────────────────
        void OnTriggerEnter(const engine::CollisionInfo& info) override;

        // ─────────────────────────────────────────────
        // 상태 조회
        // ─────────────────────────────────────────────
        bool IsDestroyed() const { return m_isDestroyed; }
        bool IsCrystallized() const { return m_isCrystallized; }
        float GetDamage() const { return m_damage; }
    };
}
