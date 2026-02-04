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
        REGISTER_SCRIPT(BulletPlayer, Script)

    private:
        // ─────────────────────────────────────────────
        // 이동 전략 (Strategy 패턴)
        // ─────────────────────────────────────────────
        std::unique_ptr<IBulletMovement> m_movement;

        // ─────────────────────────────────────────────
        // 수명/사거리 (Factory에서 전달받음)
        // ─────────────────────────────────────────────
        float m_lifetime = 3.0f;        // 하위 호환성용 (사용 안 함)
        float m_range = 50.0f;         // 사거리 (BulletPlayer는 이 값 사용)
        float m_elapsedTime = 0.0f;    // 하위 호환성용 (사용 안 함)
        
        // ─────────────────────────────────────────────
        // 사거리 기반 생명주기 관리
        // ─────────────────────────────────────────────
        engine::Vector3 m_startPosition = engine::Vector3::Zero;  // 발사 위치
        engine::Vector3 m_direction = engine::Vector3::Zero;     // 발사 방향 (정규화됨)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isDying = false;
        float m_deathDelay = 0.05f;
        float m_deathTimer = 0.0f;

        float m_damage = 10.0f;

    public:
        // ─────────────────────────────────────────────
        // 초기화 (Factory에서 호출)
        // - damage: 플레이어 강화 반영된 데미지 (미전달 시 기본값 사용)
        // - range: 사거리 (BulletPlayer는 range 사용, lifetime 무시)
        // ─────────────────────────────────────────────
        void Setup(std::unique_ptr<IBulletMovement> movement, float lifetime, float dmg, float range = 50.0f);

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
