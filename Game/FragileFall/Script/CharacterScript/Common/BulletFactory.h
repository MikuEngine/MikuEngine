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

    private:
        // ─────────────────────────────────────────────
        // 내부 헬퍼: Movement 생성
        // ─────────────────────────────────────────────
        std::unique_ptr<IBulletMovement> CreateMovement(const BulletParams& params);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
