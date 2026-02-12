#pragma once

#include <Framework/Object/Component/Script.h>
#include "BulletParams.h"
#include "BulletMovement.h"
#include <memory>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // BulletFactory - 총알 생성 Factory
    // 
    // 책임:
    //   - 총알 GameObject 생성
    //   - 컴포넌트(Renderer, Rigidbody, Collider) 설정
    //   - BulletParams에 따라 적절한 Movement 생성 및 초기화
    // 
    // 호출자는 파라미터만 전달, Factory가 모든 생성/초기화 담당
    // ═══════════════════════════════════════════════════════════════
    class BulletFactory :
        public engine::Script<BulletFactory>
    {
        REGISTER_SCRIPT(BulletFactory, Script)

    public:
        // ─────────────────────────────────────────────
        // 플레이어 총알 발사
        // ─────────────────────────────────────────────
        void Fire(const engine::Vector3& position,
                  const engine::Vector3& direction,
                  const BulletParams& params);

        // ─────────────────────────────────────────────
        // 몬스터 직선 총알 발사 (EnemyProjectile 레이어 사용)
        // ─────────────────────────────────────────────
        void LinearFireMonster(const engine::Vector3& position,
                               const engine::Vector3& direction,
                               const BulletParams& params);

        void ParabolicFireMonster(const engine::Vector3& position,
                                  const engine::Vector3& direction,
			                      const BulletParams& params);

        void FieldFireMonster(const engine::Vector3& position,
			                  const BulletParams& params);

        // ─────────────────────────────────────────────
        // 나선형 총알 발사 (4발, +X/-X/+Z/-Z 방향)
        // angularSpeed: 회전 속도 (rad/s)
        // radiusGrowthRate: 반지름 증가율 (m/s)
        // ─────────────────────────────────────────────
        void CurvedFireMonster(const engine::Vector3& position,
                               float angularSpeed,
                               float radiusGrowthRate,
                               const BulletParams& params);

        // ─────────────────────────────────────────────
        // 3방향 총알 발사 (중앙 + 좌우 퍼짐)
        // spreadAngle: 좌우 퍼짐 각도 (라디안)
        // ─────────────────────────────────────────────
        void ThreewayFireMonster(const engine::Vector3& position,
                                 const engine::Vector3& direction,
                                 float spreadAngle,
                                 const BulletParams& params);

        // ─────────────────────────────────────────────
        // 보스 3방향 총알 발사 (중앙 + 좌우 퍼짐)
        // spreadAngle: 좌우 퍼짐 각도 (라디안)
        // 프리팹: "BossBulletProjectile"
        // 레이어: BossBulletProjectile
        // ─────────────────────────────────────────────
        void ThreewayFireBoss(const engine::Vector3& position,
                              const engine::Vector3& direction,
                              float spreadAngle,
                              const BulletParams& params);

        // ─────────────────────────────────────────────
        // 보스 메테오 8방향 총알 발사 (고정 8방향)
        // 프리팹: "BossBulletProjectile"
        // 레이어: BossBulletProjectile
        // position: 메테오 XZ 좌표 (Y는 자동으로 1.5 적용)
        // ─────────────────────────────────────────────
        void EightwayFireBossMeteor(const engine::Vector3& position,
                                    const BulletParams& params);

        // ─────────────────────────────────────────────
        // 연사 총알 발사 (랜덤 각도 + 속도/수명 변조)
        // count: 발사할 총알 개수
        // spreadAngle: 좌우 퍼짐 각도 (라디안)
        // lifetimeModMin/Max: 수명 변조 범위 (0.0~1.0 배율)
        // speedModMin/Max: 속도 변조 범위 (0.0~1.0 배율)
        // ─────────────────────────────────────────────
        void BurstFireMonster(const engine::Vector3& position,
                              const engine::Vector3& direction,
                              int count,
                              float spreadAngle,
                              float lifetimeModMin,
                              float lifetimeModMax,
                              float speedModMin,
                              float speedModMax,
                              const BulletParams& params);

    private:
        // ─────────────────────────────────────────────
        // 내부 헬퍼: Movement 생성
        // ─────────────────────────────────────────────
        std::unique_ptr<IBulletMovement> CreateMovement(const BulletParams& params);

        // ─────────────────────────────────────────────
        // Monster tier 별 총알 메쉬 변경
        // ─────────────────────────────────────────────
		void SetBulletMeshByTier(engine::Ptr<engine::GameObject> bulletGO, MonsterTier tier);


    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
