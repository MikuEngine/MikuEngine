#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Common/BulletParams.h"
#include <memory>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // BulletMonster - 몬스터 총알 컴포넌트
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
    // 차이점 (vs BulletPlayer):
    //   - PhysicsLayer: EnemyProjectile 사용
    //   - 충돌 대상: PlayerControllerScript
    // 
    // 참고:
    //   - 불릿은 항상 Factory를 통해서만 생성됨
    //   - OnGui/Save/Load 없음 (씬에 저장되지 않음)
    // ═══════════════════════════════════════════════════════════════
    class BulletFactory;

    class BulletMonster :
        public engine::Script<BulletMonster>
    {
        REGISTER_SCRIPT(BulletMonster, Script)

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
        
        // ─────────────────────────────────────────────
        // 포물선 탄환 전용 (착탄 시 폭발 트리거 생성)
        // ─────────────────────────────────────────────
        engine::Vector3 m_impactPoint = engine::Vector3::Zero;  // 착탄점
        bool m_shouldSpawnExplosion = false;                     // 폭발 트리거 생성 여부

		// ─────────────────────────────────────────────
		// 필드형 총알 전용
        // ─────────────────────────────────────────────
		bool m_isFieldType = false;
		float m_radius = 0.0f;
		float m_tickTimer = 0.0f;
		float m_tickInterval = 0.5f; // 0.5초마다 데미지 적용
        engine::Ptr<engine::GameObject> m_targetPlayer = nullptr;

    public:
        // ─────────────────────────────────────────────
        // 초기화 (Factory에서 호출)
        // ─────────────────────────────────────────────
        void Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory);
		//void SetupField(float radius, const BulletParams& params);

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
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
    private:
        // ─────────────────────────────────────────────
        // 포물선 탄환 전용 헬퍼
        // ─────────────────────────────────────────────
        void SpawnExplosionTrigger(const engine::Vector3& position);
        void DieWithExplosion(const engine::Vector3& impactPoint);  // [미사용]
    };
}
