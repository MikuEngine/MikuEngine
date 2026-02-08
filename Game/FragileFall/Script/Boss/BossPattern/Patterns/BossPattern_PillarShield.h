#pragma once

#include "Script/Boss/BossPattern/BossPatternBase.h"
#include <Framework/Object/Ptr.h>

namespace game
{
    class BossScript;
    class BossPillar;

    class BossPattern_PillarShield :
        public BossPatternBase
    {
    private:
        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_respawnTimer = 0.0f;  // 재생성 대기 타이머
        std::vector<engine::Ptr<BossPillar>> m_spawnedPillars;  // 생성된 기둥들
        std::vector<engine::Ptr<engine::GameObject>> m_shieldEffects;  // 쉴드 이펙트들

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return false; }  // 지속 패턴
        float GetInterval() const override { return 0.0f; }  // 대기 방식이므로 무의미
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "PillarShield"; }
        std::string GetPatternDescription() const override { return "기둥 쉴드"; }

    private:
        // ─────────────────────────────────────────────
        // 기둥 생성 (베이직)
        // ─────────────────────────────────────────────
        void SpawnBasicPillars(BossScript* boss);

        // ─────────────────────────────────────────────
        // 기둥 생성 (랜덤 + 밀집도)
        // ─────────────────────────────────────────────
        void SpawnRandomPillars(BossScript* boss);

        // ─────────────────────────────────────────────
        // 쉴드 이펙트 생성
        // ─────────────────────────────────────────────
        void SpawnShieldEffects(BossScript* boss);

        // ─────────────────────────────────────────────
        // 겹침 체크 헬퍼
        // ─────────────────────────────────────────────
        bool IsOverlapping(const engine::Vector3& pos, float radius) const;
    };
}
