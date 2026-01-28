#pragma once

#include "Script/Boss/BossPattern/BossPatternBase.h"
#include <Framework/Object/Ptr.h>

namespace game
{
    class BossScript;
    class BossPillar;

    // ═══════════════════════════════════════════════════════════════
    // BossPattern_PillarShield - 기둥 쉴드 패턴 (독립 패턴)
    // 
    // 기능:
    //   - N초마다 기둥 생성
    //   - 기둥이 모두 파괴되지 않으면 보스에게 데미지 차단
    // 
    // 특징:
    //   - 유일한 독립 패턴 (다른 패턴들과 독립적으로 시간 기반 실행)
    // ═══════════════════════════════════════════════════════════════
    class BossPattern_PillarShield :
        public BossPatternBase
    {
    private:
        float m_interval = 10.0f;  // 10초마다 실행
        float m_intervalTimer = 0.0f;  // 간격 타이머
        int m_pillarCount = 4;  // 생성할 기둥 개수
        float m_pillarSpawnRadius = 8.0f;  // 기둥 생성 반경
        std::vector<engine::Ptr<BossPillar>> m_spawnedPillars;  // 생성된 기둥들

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
