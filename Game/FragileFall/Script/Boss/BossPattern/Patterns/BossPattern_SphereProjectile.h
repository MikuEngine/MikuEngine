#pragma once

#include "Script/Boss/BossPattern/BossPatternBase.h"

namespace game
{
    class BossScript;
    class BossBigProjectile;

    class BossPattern_SphereProjectile :
        public BossPatternBase
    {
    private:
        float m_interval = 6.0f;  // 6초마다 실행

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return true; }  // 즉시 완료
        float GetInterval() const override { return m_interval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "SphereProjectile"; }
        std::string GetPatternDescription() const override { return "구체 발사"; }

        void FireProjectile(BossScript* boss);
    };
}
