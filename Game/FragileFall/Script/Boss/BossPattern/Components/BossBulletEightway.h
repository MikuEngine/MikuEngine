#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Common/BulletParams.h"
#include <memory>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // BossBulletEightway - 보스 8방향 총알 컴포넌트
    // 
    // Strategy 패턴:
    //   - IBulletMovement로 이동 방식을 주입받음 (Linear만 사용)
    //   - BulletFactory::EightwayFireBossMeteor()에서 생성 시 Movement 설정
    // 
    // 책임:
    //   - 수명 관리 (Factory로부터 전달받은 lifetime 사용)
    //   - 충돌 처리 (OnTriggerEnter)
    //   - Movement에 위임하여 이동
    // 
    // 차이점 (vs BulletMonster):
    //   - PhysicsLayer: BossBulletProjectile 사용
    //   - 충돌 대상: PlayerControllerScript만
    //   - 항상 Linear Movement 사용
    // 
    // 참고:
    //   - 불릿은 항상 Factory를 통해서만 생성됨
    //   - OnGui/Save/Load 없음 (씬에 저장되지 않음)
    // ═══════════════════════════════════════════════════════════════
    class BulletFactory;

    class BossBulletEightway :
        public engine::Script<BossBulletEightway>
    {
        REGISTER_SCRIPT(BossBulletEightway, Script)

    private:
        // ─────────────────────────────────────────────
        // 이동 전략 (Strategy 패턴)
        // ─────────────────────────────────────────────
        std::unique_ptr<IBulletMovement> m_movement;
        BulletParams m_params;
        BulletFactory* m_cachedFactory = nullptr;

        // ─────────────────────────────────────────────
        // 수명 (Factory에서 전달받음)
        // ─────────────────────────────────────────────
        float m_lifetime = 3.0f;
        float m_elapsedTime = 0.0f;

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isDying = false;
        float m_deathDelay = 0.05f;
        float m_deathTimer = 0.0f;

    public:
        // ─────────────────────────────────────────────
        // 초기화 (Factory에서 호출)
        // ─────────────────────────────────────────────
        void Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory);

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
    };
}
