#pragma once

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/BossPatternBase.h"

namespace game
{
    class BossScript;

    class BossPattern_Summon :
        public BossPatternBase
    {
    private:
        enum class MonsterType
        {
            Type1,
            Type2,
            Type3
        };

    private:
        float m_interval = 8.0f;  // 8초마다 실행
        int m_summonCount = 3;  // 한 번에 소환할 몬스터 수
        float m_summonRadius = 10.0f;  // 소환 반경

        int GetMonsterID(MonsterType type, BossScript::BossColor color) const;

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return true; }  // 즉시 완료
        float GetInterval() const override { return m_interval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "Summon"; }
        std::string GetPatternDescription() const override { return "몬스터 소환"; }

        void SummonMonsters(BossScript* boss);
    };
}
