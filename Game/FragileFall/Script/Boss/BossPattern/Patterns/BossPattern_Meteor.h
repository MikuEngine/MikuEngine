#pragma once

#include "Script/Boss/BossPattern/BossPatternBase.h"
#include <Framework/Object/Ptr.h>

namespace engine
{
    class GameObject;
}

namespace game
{
    class BossScript;

    class BossPattern_Meteor :
        public BossPatternBase
    {
    private:
        // ─────────────────────────────────────────────
        // 현재 패턴 상태
        // ─────────────────────────────────────────────
        float m_currentInterval = 5.0f;  // 현재 인터벌 (BossScript에서 설정)

        // ─────────────────────────────────────────────
        // 활성 메테오 추적 (안전장치: 중복 생성 방지)
        // ─────────────────────────────────────────────
        engine::Ptr<engine::GameObject> m_activeMeteor = nullptr;

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return true; }
        float GetInterval() const override { return m_currentInterval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "Meteor"; }
        std::string GetPatternDescription() const override { return "운석 낙하"; }

    private:
        // ─────────────────────────────────────────────
        // 메테오 생성 (플레이어 예측 + 정확도 + 클램핑)
        // ─────────────────────────────────────────────
        void SpawnMeteor(BossScript* boss);

        // ─────────────────────────────────────────────
        // 플레이어 예측 위치 계산
        // ─────────────────────────────────────────────
        engine::Vector3 CalculatePredictedPosition(BossScript* boss) const;
    };
}
