#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/CharacterScript/Common/BulletMovement.h"
#include <memory>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // BulletPlayer - 플레이어 총알 컴포넌트
    // 
    // Strategy 패턴:
    //   - IBulletMovement로 이동 방식을 주입받음
    //   - Factory가 생성 시 적절한 Movement를 설정
    // 
    // 책임:
    //   - 수명 관리 (Factory로부터 전달받은 lifetime 사용)
    //   - 충돌 처리 (OnTriggerEnter)
    //   - Movement에 위임하여 이동
    // 
    // 참고:
    //   - 불릿은 항상 Factory를 통해서만 생성됨
    //   - OnGui/Save/Load 없음 (씬에 저장되지 않음)
    // ═══════════════════════════════════════════════════════════════
    class BulletPlayer :
        public engine::Script<BulletPlayer>
    {
        REGISTER_COMPONENT(BulletPlayer, Script)

    private:
        // ─────────────────────────────────────────────
        // 이동 전략 (Strategy 패턴)
        // ─────────────────────────────────────────────
        std::unique_ptr<IBulletMovement> m_movement;

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
        void Setup(std::unique_ptr<IBulletMovement> movement, float lifetime);

        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Start() override;
        void Update() override;

        // ─────────────────────────────────────────────
        // 충돌 콜백
        // ─────────────────────────────────────────────
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
    };
}
