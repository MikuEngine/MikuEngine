#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/Interface/IDamageable.h"

namespace engine
{
    class GameObject;
    class StaticMeshRenderer;
    class Transform;
    class UIText;
    class Camera;
}

namespace game
{
    class PlayerControllerScript;
    class BossPatternManager;
    class BossPillar;
    class BulletFactory;
    class BossSubPartsController;
    class AimModeController;

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
        // ─────────────────────────────────────────────
        // 보스 기본 설정
        // ─────────────────────────────────────────────
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

        // XZ 유효 범위 (직사각형)
        float m_meteorSpawnCenterX = 0.0f;            // 스폰 중점 X 좌표
        float m_meteorSpawnCenterZ = 0.0f;            // 스폰 중점 Z 좌표
        float m_meteorValidRangeX = 20.0f;            // 직사각형 가로 길이 (전체)
        float m_meteorValidRangeZ = 20.0f;            // 직사각형 세로 길이 (전체)

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

        // ─────────────────────────────────────────────
        // PillarShield 패턴 설정 (직렬화)
        // ─────────────────────────────────────────────
        // 패턴 모드
        bool m_pillarUseBasicPattern = true;          // true: 좌우 2개 고정

        // 재생성 대기 시간 (모든 기둥 파괴 후)
        bool m_pillarUseFixedRespawnDelay = false;    // 고정 대기 시간 사용 여부
        float m_pillarFixedRespawnDelay = 5.0f;       // 고정 대기 시간 (초)
        float m_pillarMinRespawnDelay = 3.0f;         // 최소 대기 시간 (초)
        float m_pillarMaxRespawnDelay = 10.0f;        // 최대 대기 시간 (초)

        // 생성 개수 (Use Basic = false일 때)
        bool m_pillarUseFixedCount = false;           // 고정 개수 사용 여부
        int m_pillarFixedCount = 5;                   // 고정 개수
        int m_pillarMinCount = 1;                     // 최소 개수
        int m_pillarMaxCount = 10;                    // 최대 개수

        // 생성 영역 (직사각형)
        float m_pillarSpawnCenterX = 0.0f;            // 스폰 중점 X 좌표
        float m_pillarSpawnCenterZ = 0.0f;            // 스폰 중점 Z 좌표
        float m_pillarSpawnRangeX = 20.0f;            // 직사각형 가로 길이 (전체)
        float m_pillarSpawnRangeZ = 20.0f;            // 직사각형 세로 길이 (전체)
        float m_pillarSpawnY = 0.0f;                  // Y 좌표

        // 밀집도 (0.0=분산, 1.0=밀집)
        float m_pillarClusteringStrength = 0.0f;      // 0~1

        // 겹침 방지
        float m_pillarOverlapRadius = 1.5f;           // 겹침 체크 반지름
        int m_pillarMaxSpawnAttempts = 20;            // 최대 재시도 횟수

        // 기둥 HP
        float m_pillarHP = 30.0f;                     // 기둥 HP

        // ─────────────────────────────────────────────
        // BigProjectile 패턴 설정 (직렬화)
        // ─────────────────────────────────────────────
        float m_bigProjectileHP = 1.0f;                        // 체력
        float m_bigProjectileSpeed = 12.0f;                    // 이동 속도
        float m_bigProjectileScale = 2.0f;                     // 크기 배율
        float m_bigProjectileLifetime = 10.0f;                 // 수명 (초)
        float m_bigProjectileDamage = 10.0f;                   // 플레이어 데미지
        float m_bigProjectileReflectDamage = 200.0f;           // 반사 데미지
        float m_bigProjectileReflectLifetimeExtension = 10.0f; // 반사 시 수명 연장 (초)
        float m_bigProjectileShieldReduction = 100.0f;         // 실드 감쇄율 (0~100%)
        bool m_isShieldPierce = true;                          // 실드 관통 가능 여부
        
        // 발사 위치 오프셋 (보스 좌표 기준)
        float m_bigProjectileSpawnOffsetX = 0.0f;              // X축 오프셋
        float m_bigProjectileSpawnOffsetY = 0.0f;              // Y축 오프셋
        float m_bigProjectileSpawnOffsetZ = 0.0f;              // Z축 오프셋

        // shield
        bool m_isShieldActive = false;
        std::vector<engine::Ptr<BossPillar>> m_activePillars;

        // pattern
        std::unique_ptr<BossPatternManager> m_patternManager = nullptr;
        bool m_patternsInitialized = false;

        // player
        engine::Ptr<PlayerControllerScript> m_targetPlayer = nullptr;

        engine::Ptr<engine::UIText> m_hpText = nullptr;

        bool m_canBeCrystallized = true;  // 구체 패턴의 투사체 결정화 가능 여부

        // BulletFactory (보스 GameObject 내에서 찾기)
        engine::Ptr<BulletFactory> m_bulletFactory = nullptr;

        // Boss intro sequence
        bool m_enableIntroSequence = true;
        bool m_isBattleStarted = true;
        float m_introMoveToBossDuration = 1.2f;
        float m_introHoldDuration = 0.35f;
        float m_introAssembleDuration = 1.2f;
        float m_introReturnDuration = 1.0f;
        float m_introCameraPlayerOffsetX = 0.0f;
        float m_introCameraPlayerOffsetY = 8.0f;
        float m_introCameraPlayerOffsetZ = -6.0f;
        float m_introCameraBossDistance = 8.0f;
        float m_introCameraBossHeight = 16.0f;

        enum class IntroPhase
        {
            None,
            MoveToBoss,
            HoldAtBoss,
            AssembleParts,
            ReturnToPlayer,
            Complete,
        };
        IntroPhase m_introPhase = IntroPhase::None;
        float m_introPhaseElapsed = 0.0f;
        bool m_isIntroRunning = false;
        bool m_introAssembleTriggered = false;

        engine::Ptr<BossSubPartsController> m_subPartsController = nullptr;
        engine::Ptr<engine::GameObject> m_subPartsRoot = nullptr;
        engine::Ptr<AimModeController> m_targetAimController = nullptr;
        engine::Camera* m_mainCamera = nullptr;
        engine::Transform* m_cameraTransform = nullptr;
        bool m_hasSceneStartCameraPose = false;
        engine::Vector3 m_sceneStartCameraWorldPosition = engine::Vector3::Zero;
        engine::Quaternion m_sceneStartCameraWorldRotation = engine::Quaternion::Identity;
        engine::Vector3 m_introCameraOriginalPosition = engine::Vector3::Zero;
        engine::Quaternion m_introCameraOriginalRotation = engine::Quaternion::Identity;
        engine::Vector3 m_introCameraStartPosition = engine::Vector3::Zero;
        engine::Vector3 m_introCameraBossTargetPosition = engine::Vector3::Zero;
        engine::Quaternion m_introCameraBossTargetRotation = engine::Quaternion::Identity;
        engine::Vector3 m_introLookTargetOffset = engine::Vector3(0.0f, 2.5f, 0.0f);

    public:
        BossScript();
        ~BossScript();

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void InitializePatterns();

        // pattern
        void UpdatePatternSystem(float deltaTime);
        void OnPatternStarted(const std::string& patternName);
        void OnPatternFinished(const std::string& patternName);
        void SetBattleStarted(bool started) { m_isBattleStarted = started; }
        bool IsBattleStarted() const { return m_isBattleStarted; }
        void StartIntroSequence();
        void SkipIntroSequence();

        void TakeDamage(float damage, bool isShieldPierce = false) override;
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
        void OnExecutionReflected(engine::Vector3 direction);  // 처형 반사 시 호출 (기존 하위 호환)
        void OnReflectedProjectileHit(const engine::Vector3& direction, float damage);  // 반사된 빅탄 충돌 (실드 감쇄 적용)

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
        float GetMeteorSpawnCenterX() const { return m_meteorSpawnCenterX; }
        float GetMeteorSpawnCenterZ() const { return m_meteorSpawnCenterZ; }
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

        // ─────────────────────────────────────────────
        // PillarShield 설정 Getter
        // ─────────────────────────────────────────────
        bool GetPillarUseBasicPattern() const { return m_pillarUseBasicPattern; }
        float GetPillarRespawnDelay() const;   // 랜덤 or 고정 delay 반환
        int GetPillarSpawnCount() const;       // 랜덤 or 고정 count 반환
        float GetPillarSpawnCenterX() const { return m_pillarSpawnCenterX; }
        float GetPillarSpawnCenterZ() const { return m_pillarSpawnCenterZ; }
        float GetPillarSpawnRangeX() const { return m_pillarSpawnRangeX; }
        float GetPillarSpawnRangeZ() const { return m_pillarSpawnRangeZ; }
        float GetPillarSpawnY() const { return m_pillarSpawnY; }
        float GetPillarClusteringStrength() const { return m_pillarClusteringStrength; }
        float GetPillarOverlapRadius() const { return m_pillarOverlapRadius; }
        int GetPillarMaxSpawnAttempts() const { return m_pillarMaxSpawnAttempts; }
        float GetPillarHP() const { return m_pillarHP; }

        // ─────────────────────────────────────────────
        // BigProjectile 설정 Getter
        // ─────────────────────────────────────────────
        float GetBigProjectileHP() const { return m_bigProjectileHP; }
        float GetBigProjectileSpeed() const { return m_bigProjectileSpeed; }
        float GetBigProjectileScale() const { return m_bigProjectileScale; }
        float GetBigProjectileLifetime() const { return m_bigProjectileLifetime; }
        float GetBigProjectileDamage() const { return m_bigProjectileDamage; }
        float GetBigProjectileReflectDamage() const { return m_bigProjectileReflectDamage; }
        float GetBigProjectileReflectLifetimeExtension() const { return m_bigProjectileReflectLifetimeExtension; }
        float GetBigProjectileShieldReduction() const { return m_bigProjectileShieldReduction; }
        bool GetIsShieldPierce() const { return m_isShieldPierce; }
        engine::Vector3 GetBigProjectileSpawnOffset() const 
        { 
            return engine::Vector3(m_bigProjectileSpawnOffsetX, m_bigProjectileSpawnOffsetY, m_bigProjectileSpawnOffsetZ); 
        }
        
        engine::Ptr<BulletFactory> GetBulletFactory() const { return m_bulletFactory; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CacheIntroReferences();
        void BeginIntroSequence();
        void UpdateIntroSequence(float deltaTime);
        void EndIntroSequence(bool forceComplete);
        static float EaseInOutSine(float t);
        void UpdateIntroCameraLookAtBoss();
    };
}
