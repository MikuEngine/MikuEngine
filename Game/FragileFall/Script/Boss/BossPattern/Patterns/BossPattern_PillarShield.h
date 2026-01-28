#pragma once

#include "Script/Boss/BossPattern/BossPatternBase.h"
#include <Framework/Object/Ptr.h>

namespace game
{
    class BossScript;
    class BossPillar;
    class BossShieldEffect;

    class BossPattern_PillarShield :
        public BossPatternBase
    {
    private:
        float m_interval = 10.0f;  // 10초마다 실행
        float m_intervalTimer = 0.0f;  // 간격 타이머
        int m_pillarCount = 2;  // 생성할 기둥 개수
        std::vector<engine::Ptr<BossPillar>> m_spawnedPillars;  // 생성된 기둥들
        std::vector<engine::Ptr<BossShieldEffect>> m_shieldEffects;  // 생성된 기둥들

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return false; }  // 지속 패턴
        float GetInterval() const override { return m_interval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "PillarShield"; }
        std::string GetPatternDescription() const override { return "기둥 쉴드"; }

        void SpawnPillars(BossScript* boss);
        void CheckPillarsStatus(BossScript* boss);
    };
}
