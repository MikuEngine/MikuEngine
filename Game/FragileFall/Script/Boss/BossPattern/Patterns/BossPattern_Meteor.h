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
        float m_interval = 5.0f;  // 5초마다 실행
        int m_meteorCount = 1;  // 운석 개수
        float m_meteorDamage = 50.0f;  // 운석 데미지
        float m_meteorRadius = 3.0f;  // 충돌 범위
        float m_warningTime = 1.5f;  // 경고 표시 시간
        float m_fallHeight = 10.0f;  // 운석 낙하 시작 높이
        float m_fallSpeed = 5.0f;  // 낙하 속도
        float m_spawnRadius = 15.0f;  // 운석 생성 반경 (보스 중심 기준)
        engine::Vector3 m_spawnPosMin{ -10.0f, 0.0f, -5.0f };
        engine::Vector3 m_spawnPosMax{ 10.0f, 0.0f, 5.0f };

        float m_bulletSpeed = 10.0f;  // 탄환 속도
        float m_bulletDamage = 20.0f;  // 탄환 데미지
        float m_bulletLifetime = 5.0f;  // 탄환 수명

        // 활성 운석들
        struct MeteorData
        {
            engine::Ptr<engine::GameObject> meteorGO;
            engine::Ptr<engine::GameObject> warningGO;
            engine::Vector3 targetPos;  // 낙하 목표 위치
            float elapsedTime;
            bool hasLanded;
        };
        std::vector<MeteorData> m_activeMeteors;

    public:
        void Start(BossScript* boss) override;
        void Update(BossScript* boss, float deltaTime) override;
        void End(BossScript* boss) override;

        bool IsFinished() const override { return true; }  // 즉시 완료
        float GetInterval() const override { return m_interval; }
        float GetCooldown() const override { return 0.0f; }

        std::string GetPatternName() const override { return "Meteor"; }
        std::string GetPatternDescription() const override { return "운석 낙하"; }

        void SpawnMeteors(BossScript* boss);
        void UpdateMeteors(BossScript* boss, float deltaTime);
    };
}
