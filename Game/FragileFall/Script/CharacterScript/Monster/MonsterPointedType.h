#pragma once

#include "MonsterScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace engine
{
    struct CollisionInfo;
    class GridMap;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterPointedType - 뾰족 공격 몬스터 구현
    // 
    // 특징:
    //   - 감지 거리 내에 플레이어 진입 시 추적 시작
    //   - 공격 사거리 내 진입 시 정지하고 공격
    //   - 공격 모션 중 이동 불가
    // ═══════════════════════════════════════════════════════════════

    class MonsterPointedType : public MonsterScript
    {
        REGISTER_SCRIPT(MonsterPointedType, MonsterScript)

    private:
        // SkeletalMesh + AnimFSM: 클립 이름 Idle, WalkForward, Fire (프리팹 SkeletalAnimator에 등록)

        // ─────────────────────────────────────────────
        // Pointed 고유 변수
        // ─────────────────────────────────────────────
        float m_detectionRange = 15.0f;           // 감지 거리 (공격 사거리 * 1.5)
        float m_attackAnimationDuration = 1.1f;   // 공격 모션 대기 시간 (이동 불가 시간)
        float m_attackAnimationTimer = 0.0f;      // 공격 모션 타이머
        
        bool m_isPlayerInDetectionRange = false;  // 플레이어가 감지 거리 안에 있는지
        bool m_canFire = false;                   // 발사 가능한지 (쿨타임 체크)

        // ─────────────────────────────────────────────
        // 뾰족 보라 - Flee 스테이트
        // ─────────────────────────────────────────────
		float m_fleeRange = 10.0f;				// 도망 시작 거리 (플레이어와의 거리)
		float m_safeRange = 14.0f;				// 도망 멈추는 거리 (플레이어와의 거리)
		float m_fleeSpeedMultiplier = 1.7f;	    // 도망 시 이동 속도 배율
		bool m_isPlayerInFleeRange = false;     // 플레이어가 도망 시작 거리 안에 있는지

        // 패스파인딩 기반 도망 (인스펙터에서 설정 가능)
        float m_fleeDistanceMin = 20.0f;        // 도망 위치 최소 거리
        float m_fleeDistanceMax = 70.0f;        // 도망 위치 최대 거리
        int m_fleeSafetyMargin = 1;             // 도망 위치 안전 마진 (0=없음, 1=3x3, 2=5x5)
        
        // 런타임 상태 (직렬화 불필요)
        engine::Vector3 m_fleeTargetPos = engine::Vector3::Zero;  // 현재 도망 목표 위치
        bool m_hasFleeTarget = false;           // 유효한 도망 목표가 있는지
        
        // Wall 충돌 정보 (도망 방향 우선순위 결정용)
        engine::Vector3 m_lastWallCollisionNormal = engine::Vector3::Zero;  // 마지막 벽 충돌 노말
        engine::TimePoint m_lastWallCollisionTime;                          // 마지막 벽 충돌 시간
        bool m_hasWallCollisionInfo = false;                                // 유효한 벽 충돌 정보 여부
        float m_wallCollisionInfoTimeout = 3.0f;                            // 벽 충돌 정보 유효 시간
        
        // Flee 끼임 감지용
        engine::Vector3 m_lastFleePosition = engine::Vector3::Zero;         // 마지막 Flee 위치
        engine::TimePoint m_lastFleePositionCheckTime;                      // 마지막 위치 체크 시간
        float m_fleeStuckCheckInterval = 2.0f;                              // 끼임 체크 주기 (인스펙터 조절 가능)
        float m_fleeStuckDistanceThreshold = 0.5f;                          // 끼임 판정 거리 (인스펙터 조절 가능)

        // ─────────────────────────────────────────────
        // Flee 실패 → Redemption → Laststand 시스템
        // ─────────────────────────────────────────────
        int m_fleeAttemptCount = 0;             // 패스찾기 실패 누적 (프레임당 10회씩)
        static constexpr int kMaxFleeAttempts = 120;  // 120회 실패 시 Redemption 전이
        bool m_isNoWayOut = false;              // 탈출구 없음 플래그

        // Redemption 상태 변수
        engine::Vector3 m_redemptionMoveDir = engine::Vector3::Zero;  // 이동 방향
        int m_redemptionReflectCount = 0;       // 반사 횟수 (1회 후 타이머 시작)
        static constexpr int kRedemptionMaxReflects = 1;
        static constexpr int kRedemptionPathAttempts = 60;  // 1회 반사 + 타이머 후 패스찾기 시도 횟수
        float m_redemptionSpeedMultiplier = 1.5f;  // Redemption 이동 속도 배율
        float m_redemptionDuration = 1.0f;      // Redemption 최소 지속 시간 (1회 반사 후)
        float m_redemptionTimer = 0.0f;         // Redemption 타이머 (런타임)

        // Laststand 상태 변수
        float m_laststandTimer = 0.0f;          // 패스찾기 재시도 타이머
        float m_laststandRetryInterval = 5.0f;  // 5초마다 재시도
        static constexpr int kLaststandPathAttempts = 20;  // 한번에 20회 시도
        
        // GridMap 캐시 (도망 위치 유효성 체크용)
        engine::GridMap* m_gridMap = nullptr;

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // MonsterScript 오버라이드
        // ─────────────────────────────────────────────
        void InitializeFSM() override;
        void InitializeAnimFSM() override;
        void InitializeAnimations() override;
        void InitializeBullet() override;
        
        void MoveTowardsPlayer() override;

        // 입력 처리 (FSM 파라미터 업데이트)
        void ProcessInput() override;
       
        // 상태별 행동 - 비물리 (Update에서 호출)
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
		void ExecuteFleeBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageMoveBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageStopBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageAttackBehaviorNonPhysics(float deltaTime);
        void ExecuteIdleBehaviorNonPhysics() override;
        void ExecuteFragileBehaviorNonPhysics() override;
        void ExecuteDeadBehaviorNonPhysics() override;
        
        // 상태별 행동 - 물리 (FixedUpdate에서 호출)
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        void ExecuteFleeBehaviorPhysics();
        void ExecuteEngageMoveBehaviorPhysics();
        void ExecuteEngageStopBehaviorPhysics();
        void ExecuteEngageAttackBehaviorPhysics();
        void ExecuteIdleBehaviorPhysics() override;
        void ExecuteFragileBehaviorPhysics() override;
        void ExecuteDeadBehaviorPhysics() override;

        // Redemption/Laststand 상태 행동
        void ExecuteRedemptionBehaviorNonPhysics(float deltaTime);
        void ExecuteRedemptionBehaviorPhysics();
        void ExecuteLaststandBehaviorNonPhysics(float deltaTime);
        void ExecuteLaststandBehaviorPhysics();

        // 충돌 콜백 (Redemption 반사용)
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
        // 상태 진입 콜백
        void OnStateEntered(const std::string& state) override;
        
        // 공격
        void Attack(float deltaTime) override;
        
        // 행동 제한
        bool CanMove() const override;
        bool CanAttack() const override;        
 
        // Parabolic 타입 여부 (Green = true) 
        bool IsParabolicBullet() const override { return m_monsterTier == MonsterTier::Green; }

        // 헬퍼 함수
        bool IsPlayerInDetectionRange() const;

        // Y축 회전 (길찾기/Engage 시 방향 전환용, BaseControllerScript 로직 무관)
        void RotateTowardsDirection(const engine::Vector3& targetDirection);

        // 뾰족 보라 - 패스파인딩 기반 도망
        bool TrySelectFleeTarget();             // 도망 위치 선정 시도 (최대 10회)
        void MoveToFleeTarget();                // 도망 위치로 패스파인딩 이동
        bool IsPositionSafeForFlee(const engine::Vector3& position) const;  // 도망 위치 안전 체크 (마진 포함)

        // Redemption 반사 헬퍼
        engine::Vector3 SnapNormalToAxis(const engine::Vector3& normal) const;
        engine::Vector3 ReflectDirection(const engine::Vector3& direction, const engine::Vector3& normal) const;
        bool TryFindPathAfterRedemption();      // Redemption 후 패스찾기 (60회)

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
