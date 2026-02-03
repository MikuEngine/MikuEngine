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
    //   - Fragile 부활 시 Idle로 전이
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundBlue : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundBlue, MonsterRoundType)

    protected:
        // ─────────────────────────────────────────────────
        // 곡선 이동 변수
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
        bool m_collisionOccurred = false;         // 충돌 발생 플래그
        
        // ─────────────────────────────────────────────────
        // 플레이어 데미지 쿨다운
        // ─────────────────────────────────────────────────
        float m_damageCooldown = 1.0f;            // 데미지 쿨다운 시간 (초)
        engine::TimePoint m_lastDamageTime;       // 마지막 데미지 준 시간

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
        void InitializeFSM() override;    // Blue 전용 FSM (Engage 상태 없음, 나중에 추가)
        void ProcessInput() override;     // Blue는 현재 감지 로직 없음 (나중에 추가)
        
        // ─────────────────────────────────────────────────
        // 상태별 행동 오버라이드
        // ─────────────────────────────────────────────────
        void ExecuteIdleMoveBehaviorNonPhysics(float deltaTime) override;
        void ExecuteIdleMoveBehaviorPhysics() override;
        
        // ─────────────────────────────────────────────────
        // IdleMove 헬퍼 함수
        // ─────────────────────────────────────────────────
        void InitializeIdleMove();                              // IdleMove 초기화 (완전 랜덤 방향)
        void ResetRoamingParameters();                          // Roaming 파라미터 재설정
        void ChangeDirectionOnCollision();                      // 충돌 시 90~180도 방향 전환
        engine::Vector3 GetDirectionVector() const;             // 현재 각도 → 방향 벡터
        
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
