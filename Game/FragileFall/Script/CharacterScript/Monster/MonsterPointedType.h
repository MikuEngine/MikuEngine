#pragma once

#include "MonsterScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace engine
{
    struct CollisionInfo;
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
        // ─────────────────────────────────────────────
        // 총알 설정
        // ─────────────────────────────────────────────
        BulletParams m_bulletParams;

        // 뾰족 타입은 StaticMesh를 사용하므로 애니메이션 이름 변수 불필요

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
        
        // 런타임 상태 (직렬화 불필요)
        engine::Vector3 m_fleeTargetPos = engine::Vector3::Zero;  // 현재 도망 목표 위치
        bool m_hasFleeTarget = false;           // 유효한 도망 목표가 있는지

        // ─────────────────────────────────────────────
        // Flee 실패 → Redemption → Laststand 시스템
        // ─────────────────────────────────────────────
        int m_fleeAttemptCount = 0;             // 패스찾기 실패 누적 (프레임당 10회씩)
        static constexpr int kMaxFleeAttempts = 120;  // 120회 실패 시 Redemption 전이
        bool m_isNoWayOut = false;              // 탈출구 없음 플래그

        // Redemption 상태 변수
        engine::Vector3 m_redemptionMoveDir = engine::Vector3::Zero;  // 이동 방향
        int m_redemptionReflectCount = 0;       // 반사 횟수 (2회 후 패스찾기)
        static constexpr int kRedemptionMaxReflects = 2;
        static constexpr int kRedemptionPathAttempts = 60;  // 2회 반사 후 패스찾기 시도 횟수
        float m_redemptionSpeedMultiplier = 1.5f;  // Redemption 이동 속도 배율

        // Laststand 상태 변수
        float m_laststandTimer = 0.0f;          // 패스찾기 재시도 타이머
        float m_laststandRetryInterval = 5.0f;  // 5초마다 재시도
        static constexpr int kLaststandPathAttempts = 20;  // 한번에 20회 시도

        // 맵 경계 (도망 위치 선정용, 인스펙터에서 설정 가능)
        float m_mapBoundXMin = -29.5f;
        float m_mapBoundXMax = 29.5f;
        float m_mapBoundZMin = -18.5f;
        float m_mapBoundZMax = 19.5f;

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
        
        // 헬퍼 함수
        bool IsPlayerInDetectionRange() const;

        // 뾰족 보라 - 패스파인딩 기반 도망
        bool TrySelectFleeTarget();             // 도망 위치 선정 시도 (최대 10회)
        void MoveToFleeTarget();                // 도망 위치로 패스파인딩 이동

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
