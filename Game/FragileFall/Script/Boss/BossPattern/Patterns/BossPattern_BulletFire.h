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
        float m_interval = 3.0f;  // 3초마다 실행
        int m_bulletCount = 3;  // 한 번에 발사할 탄환 수
        float m_bulletSpeed = 10.0f;  // 탄환 속도
        float m_bulletDamage = 20.0f;  // 탄환 데미지
        float m_spreadAngle = 30.0f;  // 탄환 분산 각도 (도)
        float m_bulletLifetime = 5.0f;  // 탄환 수명

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return true; }  // 즉시 완료
        float GetInterval() const override { return m_interval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "BulletFire"; }
        std::string GetPatternDescription() const override { return "탄환 발사"; }

        void FireBullets(BossScript* boss);
    };
}
