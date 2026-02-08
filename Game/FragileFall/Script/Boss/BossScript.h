#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/Interface/IDamageable.h"

namespace engine
{
    class StaticMeshRenderer;
    class Transform;
    class UIText;
}

namespace game
{
    class PlayerControllerScript;
    class BossPatternManager;
    class BossPillar;
    class BulletFactory;

    class BossScript :
        public engine::Script<BossScript>,
        public IDamageable
    {
        REGISTER_SCRIPT(BossScript, Script)

    public:
        enum class BossColor
        {
            Red,
            Blue,
            Green,
            Yellow,
            Purple
        };

    private:
        float m_maxHp = 300.0f;
        float m_currentHp = 300.0f;

        BossColor m_currentColor = BossColor::Red;

        // ─────────────────────────────────────────────
        // BulletFire 패턴 설정 (직렬화)
        // ─────────────────────────────────────────────
        // 발사 주기 (Interval)
        bool m_bulletFireUseFixedInterval = false;    // 고정 인터벌 사용 여부
        float m_bulletFireFixedInterval = 3.0f;       // 고정 인터벌 값 (초)
        float m_bulletFireMinInterval = 2.0f;         // 최소 인터벌 (초)
        float m_bulletFireMaxInterval = 5.0f;         // 최대 인터벌 (초)

        // 탄퍼짐 각도 (Spread Angle, Degree 저장)
        bool m_bulletFireUseFixedSpread = false;      // 고정 퍼짐 각도 사용 여부
        float m_bulletFireFixedSpread = 30.0f;        // 고정 퍼짐 각도 (도)
        float m_bulletFireMinSpread = 15.0f;          // 최소 퍼짐 각도 (도)
        float m_bulletFireMaxSpread = 45.0f;          // 최대 퍼짐 각도 (도)

        // 탄환 속성
        float m_bulletFireSpeed = 10.0f;              // 탄환 속도 (m/s)
        float m_bulletFireScale = 1.0f;               // 탄환 크기 배율
        float m_bulletFireDamage = 20.0f;             // 탄환 데미지
        float m_bulletFireLifetime = 5.0f;            // 탄환 수명 (초)

        // 발사 위치 오프셋 (보스 좌표 기준)
        float m_bulletFireSpawnOffsetX = 0.0f;        // X축 오프셋
        float m_bulletFireSpawnOffsetZ = 0.0f;        // Z축 오프셋

        // ─────────────────────────────────────────────
        // Meteor 패턴 설정 (직렬화)
        // ─────────────────────────────────────────────
        // 메테오 스폰
        float m_meteorSpawnHeight = 10.0f;            // 메테오 스폰 높이 (Y 좌표)
        
        // 메테오 물리
        float m_meteorInitialSpeed = 5.0f;            // 메테오 초기 하방 속도 (m/s)
        float m_meteorOwnGravity = 0.0f;              // 메테오 자체 중력 가속도 (m/s²), 0.0=등속

        // 착지 판정
        float m_meteorLandingY = 0.0f;                // 착지 기준 Y 좌표
        float m_meteorLandingThreshold = 0.0f;        // 착지 임계값 (+방향만 적용)

        // XZ 유효 범위 (직사각형, 중앙 0,0 기준)
        float m_meteorValidRangeX = 20.0f;            // X축 유효 범위 (±)
        float m_meteorValidRangeZ = 20.0f;            // Z축 유효 범위 (±)

        // 예측 설정
        float m_meteorPredictionStrength = 1.0f;      // 예측 강도 (0.0=현재 위치, 1.0=풀 예측)
        float m_meteorPredictionAccuracy = 0.0f;      // 예측 정확도 (0=정확, 10=최대 ±5m)

        // 8방향 총알 속성
        float m_meteorBulletSpeed = 8.0f;             // 총알 속도 (m/s)
        float m_meteorBulletDamage = 15.0f;           // 총알 데미지
        float m_meteorBulletLifetime = 4.0f;          // 총알 수명 (초)
        float m_meteorBulletScale = 1.0f;             // 총알 크기 배율

        // 폭발 속성
        float m_meteorExplosionDamage = 30.0f;        // 폭발 데미지
        float m_meteorExplosionRadius = 5.0f;         // 폭발 반경 (m)
        float m_meteorExplosionLifetime = 0.5f;       // 폭발 지속 시간 (초)

        // 메테오 스폰 주기
        bool m_meteorUseFixedInterval = false;        // 고정 인터벌 사용 여부
        float m_meteorFixedInterval = 5.0f;           // 고정 인터벌 값 (초)
        float m_meteorMinInterval = 3.0f;             // 최소 인터벌 (초)
        float m_meteorMaxInterval = 7.0f;             // 최대 인터벌 (초)

        // 메테오 크기
        float m_meteorScale = 1.0f;                   // 메테오 크기 배율

        // shield
        bool m_isShieldActive = false;
        std::vector<engine::Ptr<BossPillar>> m_activePillars;

        // pattern
        std::unique_ptr<BossPatternManager> m_patternManager = nullptr;

        // 간단한 움직임용 수정 게임오브젝트
        std::vector<engine::Ptr<engine::GameObject>> m_crystalMeshGameObjects;  // 보스 수정 메쉬들

        // player
        engine::Ptr<PlayerControllerScript> m_targetPlayer = nullptr;

        engine::Ptr<engine::UIText> m_hpText = nullptr;

        bool m_canBeCrystallized = true;  // 구체 패턴의 투사체 결정화 가능 여부

        // BulletFactory (보스 GameObject 내에서 찾기)
        engine::Ptr<BulletFactory> m_bulletFactory = nullptr;

    public:
        BossScript();
        ~BossScript();

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void InitializePatterns();
        void InitializeCrystalMeshes();  // 수정 메쉬들 초기화 및 회전 설정

        // pattern
        void UpdatePatternSystem(float deltaTime);
        void OnPatternStarted(const std::string& patternName);
        void OnPatternFinished(const std::string& patternName);

        void TakeDamage(float damage) override;
        void CheckHealth();
        void OnDeath();

        // shield
        void UpdateShieldStatus();  // 기둥 상태 체크하여 쉴드 활성화/비활성화
        bool IsShieldActive() const { return m_isShieldActive; }
        void OnPillarCreated(engine::Ptr<BossPillar> pillar);
        void OnPillarDestroyed(engine::Ptr<BossPillar> pillar);

        void SetColor(BossColor color);
        BossColor GetCurrentColor() const { return m_currentColor; }
        std::string GetColorName() const;

        float GetCurrentHP() const { return m_currentHp; }
        float GetMaxHP() const { return m_maxHp; }

        engine::Ptr<PlayerControllerScript> GetTargetPlayer() const;

        void OnCrystallized();  // 결정화 상태 진입
        void OnExecutionReflected(engine::Vector3 direction);  // 처형 반사 시 호출

        void UpdateCrystalMovement(float deltaTime);  // 수정 메쉬들 회전 업데이트

        // ─────────────────────────────────────────────
        // BulletFire 설정 Getter
        // ─────────────────────────────────────────────
        float GetBulletFireInterval() const;   // 랜덤 or 고정 interval 반환
        float GetBulletFireSpread() const;     // 랜덤 or 고정 spread (Radian 변환) 반환
        float GetBulletFireSpeed() const { return m_bulletFireSpeed; }
        float GetBulletFireScale() const { return m_bulletFireScale; }
        float GetBulletFireDamage() const { return m_bulletFireDamage; }
        float GetBulletFireLifetime() const { return m_bulletFireLifetime; }
        engine::Vector3 GetBulletFireSpawnOffset() const { return engine::Vector3(m_bulletFireSpawnOffsetX, 0.0f, m_bulletFireSpawnOffsetZ); }

        // ─────────────────────────────────────────────
        // Meteor 설정 Getter
        // ─────────────────────────────────────────────
        float GetMeteorInterval() const;       // 랜덤 or 고정 interval 반환
        float GetMeteorSpawnHeight() const { return m_meteorSpawnHeight; }
        float GetMeteorInitialSpeed() const { return m_meteorInitialSpeed; }
        float GetMeteorOwnGravity() const { return m_meteorOwnGravity; }
        float GetMeteorLandingY() const { return m_meteorLandingY; }
        float GetMeteorLandingThreshold() const { return m_meteorLandingThreshold; }
        float GetMeteorValidRangeX() const { return m_meteorValidRangeX; }
        float GetMeteorValidRangeZ() const { return m_meteorValidRangeZ; }
        float GetMeteorPredictionStrength() const { return m_meteorPredictionStrength; }
        float GetMeteorPredictionAccuracy() const { return m_meteorPredictionAccuracy; }
        float GetMeteorBulletSpeed() const { return m_meteorBulletSpeed; }
        float GetMeteorBulletDamage() const { return m_meteorBulletDamage; }
        float GetMeteorBulletLifetime() const { return m_meteorBulletLifetime; }
        float GetMeteorBulletScale() const { return m_meteorBulletScale; }
        float GetMeteorExplosionDamage() const { return m_meteorExplosionDamage; }
        float GetMeteorExplosionRadius() const { return m_meteorExplosionRadius; }
        float GetMeteorExplosionLifetime() const { return m_meteorExplosionLifetime; }
        float GetMeteorScale() const { return m_meteorScale; }
        
        engine::Ptr<BulletFactory> GetBulletFactory() const { return m_bulletFactory; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
