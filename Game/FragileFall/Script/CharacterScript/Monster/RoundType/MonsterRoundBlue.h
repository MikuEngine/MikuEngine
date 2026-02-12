#pragma once

#include "MonsterRoundType.h"

namespace engine
{
    struct CollisionInfo;
    class GridMap;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundBlue - Blue 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Blue 고정
    //   - IdleMove: PathfindingAgent 기반 진동 이동
    //     • PA로 플레이어 방향 6~12m 목표 설정
    //     • waypoint를 따라가며 사인파 진동 (진폭 0.5~3.0 랜덤)
    //     • waypoint 도달 시 다음 waypoint로
    //     • 최종 목표 도달 또는 7초 경과 시 새 목표 설정
    //     • 충돌 시: 진동 정지 → 노말 기반 90도 × 2m 반사 → 새 목표 설정
    //   - EngageMove: 플레이어 감지 시 고정 목표로 돌진 (PA 비활성화)
    //   - EngageCollision: 충돌로 돌진 종료, 회전하며 감속
    //   - EngageArrival: 목표 도달로 돌진 종료, 직진하며 감속
    //   - Round 타입: Fragile 없이 바로 Dead로 전이
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundBlue : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundBlue, MonsterRoundType)

    protected:
        // ─────────────────────────────────────────────────
        // IdleMove 진동 이동 변수 (PathfindingAgent 기반)
        // ─────────────────────────────────────────────────
        
        // 목표 및 타이머
        engine::Vector3 m_idleMoveTargetPosition = engine::Vector3::Zero;  // PA가 계산한 최종 목표
        float m_idleMoveTimer = 0.0f;                      // 이동 경과 시간
        float m_idleMoveTimeLimit = 7.0f;                  // 시간 제한 (직렬화)
        float m_targetReachDistance = 2.0f;                // waypoint 도달 판정 거리 (직렬화)
        bool m_hasIdleMoveTarget = false;                  // 유효한 목표 존재 여부
        
        // PA 목표 거리 설정
        float m_targetDistanceMin = 6.0f;                  // 플레이어 방향 최소 거리 (직렬화)
        float m_targetDistanceMax = 12.0f;                 // 플레이어 방향 최대 거리 (직렬화)
        int m_targetSafetyMargin = 1;                      // 목표 위치 안전 마진 (직렬화, 0=없음, 1=3x3, 2=5x5)
        
        // 진동 설정 (사인파)
        float m_oscillationAmplitudeMin = 0.5f;            // 진폭 최소값 (직렬화)
        float m_oscillationAmplitudeMax = 3.0f;            // 진폭 최대값 (직렬화)
        float m_oscillationSpeedMultiplier = 5.0f;         // 진동 속도 배율 (직렬화, 좌우 이동 속도 제어)
        
        // 진동 런타임
        float m_currentOscillationAmplitude = 0.0f;        // 현재 파장의 진폭
        float m_oscillationPhase = 0.0f;                   // 사인파 위상 (0~2π)
        engine::Vector3 m_currentWaypointTarget = engine::Vector3::Zero;  // 현재 waypoint
        engine::Vector3 m_idleMoveLastPosition = engine::Vector3::Zero;   // 경로 정체 감지용 이전 위치
        float m_idleMoveStuckTimer = 0.0f;                 // 이동 정체 누적 시간
        float m_idleMoveStuckThreshold = 1.0f;             // 정체 판정 시간 (초)
        float m_idleMoveMinMoveDistance = 0.05f;           // 정체 해제 최소 이동거리 (m)
        
        // 충돌 반사 이동
        float m_collisionReflectDistance = 2.0f;           // 충돌 시 반사 이동 거리 (직렬화)
        bool m_isReflecting = false;                       // 반사 이동 중인지
        float m_reflectTimer = 0.0f;                       // 반사 이동 경과 시간
        float m_reflectDuration = 0.5f;                    // 반사 이동 지속 시간 (하드코딩)
        engine::Vector3 m_reflectDirection = engine::Vector3::Zero;  // 반사 방향
        
        // GridMap 캐시 (목표 위치 안전성 체크용)
        engine::GridMap* m_gridMap = nullptr;
        
        // ─────────────────────────────────────────────────
        // 충돌 처리용 플래그
        // ─────────────────────────────────────────────────
        bool m_collisionOccurred = false;         // IdleMove 충돌 발생 플래그
        bool m_engageCollisionOccurred = false;   // EngageMove 충돌 발생 플래그 (Player 포함 모든 충돌 대상)
        bool m_engageArrivalOccurred = false;     // EngageMove 목표 도달 플래그
        bool m_transitionCollisionOccurred = false;  // EngageCollision/EngageArrival 중 충돌 플래그
        
        // ─────────────────────────────────────────────────
        // 충돌 방향 저장 (EngageCollision용)
        // ─────────────────────────────────────────────────
        engine::Vector3 m_lastCollisionNormal = engine::Vector3::Zero;  // 마지막 충돌 노말
        
        // ─────────────────────────────────────────────────
        // 플레이어 데미지 쿨다운
        // ─────────────────────────────────────────────────
        float m_damageCooldown = 1.0f;            // 데미지 쿨다운 시간 (초)
        engine::TimePoint m_lastDamageTime;       // 마지막 데미지 준 시간
        
        // ─────────────────────────────────────────────────
        // EngageMove 설정 (에디터 직렬화)
        // ─────────────────────────────────────────────────
        float m_engageMoveSpeed = 10.0f;          // 돌진 속도 (직렬화)
        float m_engageTargetMultiplier = 1.1f;    // 목표 거리 배율 (하드코딩 예정)
        float m_engageArrivalThreshold = 0.5f;    // 목표 도달 판정 거리 (하드코딩 예정)
        
        // ─────────────────────────────────────────────────
        // EngageMove 런타임 변수
        // ─────────────────────────────────────────────────
        engine::Vector3 m_engageTargetPosition = engine::Vector3::Zero;  // 돌진 목표 위치
        engine::Vector3 m_engageDirection = engine::Vector3::Zero;       // 돌진 방향 (정규화)
        bool m_hasEngageTarget = false;           // 유효한 목표가 있는지
        float m_engageMoveSpeedScaled = 10.0f;    // 예상 충돌 거리 기반 EngageMove 속도
        
        // ─────────────────────────────────────────────────
        // EngageCollision / EngageArrival 설정
        // ─────────────────────────────────────────────────
        float m_engageTransitionDuration = 2.0f;  // 전이 상태 감속 시간 (초)
        float m_engageTransitionTimer = 0.0f;     // 전이 상태 타이머
        engine::Vector3 m_transitionMoveDirection = engine::Vector3::Zero;  // 전이 상태 이동 방향
        float m_transitionStartSpeed = 0.0f;           // 전이 시작 속도
        float m_transitionCollisionBrakeMultiplier = 3.0f;  // 전이 중 충돌 시 감속 가속 배율
        
        // ─────────────────────────────────────────────────
        // 플레이어 무시 시스템 (Idle 진입 후)
        // ─────────────────────────────────────────────────
        float m_playerIgnoreDuration = 1.0f;      // 플레이어 무시 시간 (초, 직렬화)
        float m_playerIgnoreTimer = 0.0f;         // 무시 타이머 경과
        bool m_isIgnoringPlayer = false;          // 현재 플레이어 무시 중인지

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────────
        // 충돌 콜백 (Script.h의 가상 함수 오버라이드)
        // ─────────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
        // ─────────────────────────────────────────────────
        // 오버라이드 (Blue 전용 로직)
        // ─────────────────────────────────────────────────
        void InitializeFSM() override;      // Blue 전용 FSM
        void InitializeBullet() override;   // 총알 초기화 (Dead 시 발사용)
        void ProcessInput() override;       // 플레이어 감지 + 무시 타이머
        
        // ─────────────────────────────────────────────────
        // 상태별 행동 오버라이드
        // ─────────────────────────────────────────────────
        void ExecuteIdleMoveBehaviorNonPhysics(float deltaTime) override;
        void ExecuteIdleMoveBehaviorPhysics() override;
        void ExecuteEngageMoveBehaviorNonPhysics(float deltaTime) override;
        void ExecuteEngageMoveBehaviorPhysics() override;
        
        // ─────────────────────────────────────────────────
        // EngageCollision / EngageArrival 상태 행동
        // (MonsterRoundType에 없으므로 UpdateStateBasedBehavior에서 직접 호출)
        // ─────────────────────────────────────────────────
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        
        void ExecuteEngageCollisionBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageCollisionBehaviorPhysics();
        void ExecuteEngageArrivalBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageArrivalBehaviorPhysics();
        
        // ─────────────────────────────────────────────────
        // IdleMove 헬퍼 함수 (진동 이동)
        // ─────────────────────────────────────────────────
        void InitializeIdleMove();                              // IdleMove 초기화 (PA로 목표 설정)
        bool TrySetIdleMoveTarget();                            // PA로 목표 위치 설정 시도 (안전 마진 체크 포함)
        bool IsPositionSafeForIdleMove(const engine::Vector3& position) const;  // 목표 위치 안전성 체크 (GridMap + 마진)
        void UpdateOscillationMovement();                       // 진동하며 waypoint로 이동
        void ResetOscillationPhase();                           // 진동 위상 초기화 (새 파장 시작)
        bool HasReachedCurrentWaypoint() const;                 // 현재 waypoint 도달 여부
        void AdvanceToNextWaypoint();                           // 다음 waypoint로 전환
        bool IsIdleMoveComplete() const;                        // IdleMove 완료 여부 (목표 도달 또는 시간 초과)
        void HandleIdleMoveCollision(const engine::Vector3& collisionNormal);  // 충돌 처리 → 반사 이동
        engine::Vector3 CalculateReflectDirection(const engine::Vector3& collisionNormal) const;  // 노말 기반 90도 반사 방향
        
        // ─────────────────────────────────────────────────
        // EngageMove 헬퍼 함수
        // ─────────────────────────────────────────────────
        void InitializeEngageMove();                            // EngageMove 초기화 (목표 위치 계산)
        bool HasReachedEngageTarget() const;                    // 목표 도달 여부 확인
        void UpdateEngageMoveSpeedScale(float engageMoveRange); // 예상 충돌 거리 기반 속도 스케일 계산
        
        // ─────────────────────────────────────────────────
        // EngageCollision / EngageArrival 헬퍼 함수
        // ─────────────────────────────────────────────────
        void InitializeEngageCollision();                       // EngageCollision 초기화
        void InitializeEngageArrival();                         // EngageArrival 초기화
        float CalculateTransitionSpeed() const;                 // 감속 중 현재 속도 계산
        
        // ─────────────────────────────────────────────────
        // 플레이어 무시 시스템
        // ─────────────────────────────────────────────────
        void StartPlayerIgnore();                               // 무시 시작 (Idle 진입 시)
        void UpdatePlayerIgnoreTimer(float deltaTime);          // 무시 타이머 업데이트
        bool CanDetectPlayer() const;                           // 플레이어 감지 가능 여부
        bool CanEnterEngageByRaycast() const;                   // 플레이어 직선 시야(장애물 차단) 확인
        
        // ─────────────────────────────────────────────────
        // 상태 진입 콜백 오버라이드
        // ─────────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;

    public:
        // ─────────────────────────────────────────────
        // 총알 발사 없음 (돌진 공격만, 사망 시 3방향 발사)
        // ─────────────────────────────────────────────
        bool HasBulletAttack() const override { return false; }
        
        // ─────────────────────────────────────────────
        // 컴포넌트 검증 (StaticMesh 사용, PathfindingAgent 필요)
        // ─────────────────────────────────────────────
        bool RequiresSkeletalAnimator() const override { return false; }
        bool RequiresAnimFSM() const override { return false; }
        bool RequiresPathfindingAgent() const override { return true; }
        
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
