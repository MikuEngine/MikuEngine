#pragma once

#include "MonsterRoundType.h"

namespace engine
{
    struct CollisionInfo;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundBlue - Blue 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Blue 고정
    //   - IdleMove: 곡선 이동 (매 프레임 각도 누적)
    //   - 초기 방향: 완전 랜덤 (0~360도)
    //   - m_roamingDuration 동안 좌/우 방향으로 m_turnScale만큼 회전하며 이동
    //   - 충돌 시 90~180도 랜덤 방향 전환
    //   - EngageMove: 플레이어 감지 시 고정 목표로 돌진
    //   - EngageCollision: 충돌로 돌진 종료, 회전하며 감속
    //   - EngageArrival: 목표 도달로 돌진 종료, 직진하며 감속
    //   - Fragile 부활 시 Idle로 전이
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundBlue : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundBlue, MonsterRoundType)

    protected:
        // ─────────────────────────────────────────────────
        // 곡선 이동 변수 (IdleMove)
        // ─────────────────────────────────────────────────
        float m_currentAngle = 0.0f;              // 현재 진행 방향 각도 (라디안)
        int m_turnDirection = 1;                  // 회전 방향 (+1: 좌, -1: 우)
        
        // ─────────────────────────────────────────────────
        // Roaming 설정 (에디터 직렬화)
        // ─────────────────────────────────────────────────
        float m_roamingDuration = 0.0f;           // 현재 배회 지속 시간
        float m_roamingTimer = 0.0f;              // 배회 경과 시간
        
        float m_roamingDurationMin = 1.0f;        // 배회 지속 시간 최소 (직렬화)
        float m_roamingDurationMax = 3.0f;        // 배회 지속 시간 최대 (직렬화)
        
        // ─────────────────────────────────────────────────
        // Turn Scale 설정 (에디터 직렬화)
        // - 초당 회전 각도 (도/초)
        // ─────────────────────────────────────────────────
        float m_turnScale = 0.0f;                 // 현재 회전 속도 (도/초)
        float m_turnScaleMin = 10.0f;             // 회전 속도 최소 (직렬화)
        float m_maxTurnScale = 45.0f;             // 회전 속도 최대 (직렬화)
        
        // ─────────────────────────────────────────────────
        // 충돌 처리용 플래그
        // ─────────────────────────────────────────────────
        bool m_collisionOccurred = false;         // IdleMove 충돌 발생 플래그
        bool m_engageCollisionOccurred = false;   // EngageMove 충돌 발생 플래그 (Player/Wall)
        bool m_engageArrivalOccurred = false;     // EngageMove 목표 도달 플래그
        
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
        
        // ─────────────────────────────────────────────────
        // EngageCollision / EngageArrival 설정
        // ─────────────────────────────────────────────────
        float m_engageTransitionDuration = 1.0f;  // 전이 상태 지속 시간 (1초, 하드코딩)
        float m_engageTransitionTimer = 0.0f;     // 전이 상태 타이머
        
        // EngageCollision 전용
        float m_collisionInitialAngleOffset = 45.0f;   // 충돌 반대방향에서 초기 꺾임 각도 (도, 하드코딩)
        float m_collisionRotationAmount = 30.0f;       // 1초간 추가 회전 각도 (도, 하드코딩)
        int m_collisionTurnDirection = 1;              // 회전 방향 (+1: 좌, -1: 우)
        float m_collisionStartAngle = 0.0f;            // 상태 진입 시 초기 각도
        
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
        void InitializeFSM() override;    // Blue 전용 FSM
        void ProcessInput() override;     // 플레이어 감지 + 무시 타이머
        
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
        // IdleMove 헬퍼 함수
        // ─────────────────────────────────────────────────
        void InitializeIdleMove();                              // IdleMove 초기화 (완전 랜덤 방향)
        void ResetRoamingParameters();                          // Roaming 파라미터 재설정
        void ChangeDirectionOnCollision();                      // 충돌 시 90~180도 방향 전환
        engine::Vector3 GetDirectionVector() const;             // 현재 각도 → 방향 벡터
        
        // ─────────────────────────────────────────────────
        // EngageMove 헬퍼 함수
        // ─────────────────────────────────────────────────
        void InitializeEngageMove();                            // EngageMove 초기화 (목표 위치 계산)
        bool HasReachedEngageTarget() const;                    // 목표 도달 여부 확인
        
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
        
        // ─────────────────────────────────────────────────
        // 상태 진입 콜백 오버라이드
        // ─────────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
