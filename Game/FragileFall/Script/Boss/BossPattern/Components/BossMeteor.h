#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>

namespace game
{
    class BossScript;

    // ═══════════════════════════════════════════════════════════════
    // BossMeteor - 보스 메테오 스크립트
    // 
    // 책임:
    //   - 자체 중력으로 낙하
    //   - Y 착지 판정 + 플레이어 충돌 감지
    //   - 폭발 처리 (ExplosionDamageTrigger + 8way 총알)
    // 
    // 설정값:
    //   - BossScript로부터 Getter로 가져옴
    //   - 인스펙터 설정은 모두 BossScript에 있음
    // 
    // 참고:
    //   - OnGui/Save/Load 없음 (런타임 생성만)
    // ═══════════════════════════════════════════════════════════════
    class BossMeteor :
        public engine::Script<BossMeteor>
    {
        REGISTER_SCRIPT(BossMeteor, Script)

    private:
        // ─────────────────────────────────────────────
        // 초기화 데이터
        // ─────────────────────────────────────────────
        engine::Ptr<BossScript> m_boss;
        engine::Ptr<engine::GameObject> m_warningGO;
        engine::Vector3 m_targetLandingPos = engine::Vector3::Zero;

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        bool m_hasExploded = false;
        engine::Rigidbody* m_rigidbody = nullptr;

    public:
        // ─────────────────────────────────────────────
        // 초기화 (패턴에서 호출)
        // ─────────────────────────────────────────────
        void Setup(BossScript* boss, 
                   const engine::Vector3& targetLandingPos,
                   engine::GameObject* warningGO);

        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Start() override;
        void Update() override;
        void FixedUpdate() override;

        // ─────────────────────────────────────────────
        // 충돌 콜백
        // ─────────────────────────────────────────────
        void OnTriggerEnter(const engine::CollisionInfo& info) override;

    private:
        // ─────────────────────────────────────────────
        // 폭발 처리
        // ─────────────────────────────────────────────
        void Explode();
    };
}
