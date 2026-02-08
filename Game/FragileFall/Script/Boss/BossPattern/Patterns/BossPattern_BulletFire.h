#pragma once

#include "Script/Boss/BossPattern/BossPatternBase.h"

namespace game
{
    class BossScript;
    class BossBullet;

    class BossPattern_BulletFire :
        public BossPatternBase
    {
    private:
        float m_currentInterval = 3.0f;  // 현재 interval (랜덤 or 고정)

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return true; }  // 즉시 완료
        float GetInterval() const override { return m_currentInterval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "BulletFire"; }
        std::string GetPatternDescription() const override { return "탄환 발사"; }

        void FireBullets(BossScript* boss);
    };
}
