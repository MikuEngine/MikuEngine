#pragma once

#include "MonsterRoundType.h"

namespace engine
{
    struct CollisionInfo;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundGray - Gray 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Gray 고정
    //   - IdleMove: 4방향 랜덤 이동 (월드 좌표 기준 ±X, ±Z)
    //   - 충돌 시 90도 방향 전환 (좌/우 랜덤)
    //   - 4방향 레이캐스트로 플레이어 감지
    //   - EngageMove 충돌 후 IdleMove 복귀 시 플레이어 무시 시스템
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundGray : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundGray, MonsterRoundType)

    public:
        // ─────────────────────────────────────────────
        // 이동 방향 열거형 (월드 좌표 기준)
        // ─────────────────────────────────────────────
        enum class MoveDirection
        {
            PlusX,   // +X (우)
            MinusX,  // -X (좌)
            PlusZ,   // +Z (상)
            MinusZ   // -Z (하)
        };

    protected:
        // ─────────────────────────────────────────────
        // IdleMove 상태 변수
        // ─────────────────────────────────────────────
        MoveDirection m_currentDirection = MoveDirection::PlusX;
        float m_moveDuration = 0.0f;              // 현재 이동 지속 시간
        float m_moveDurationTimer = 0.0f;         // 이동 경과 시간
        
        float m_moveDurationMin = 1.0f;           // 이동 지속 시간 최소 (직렬화)
        float m_moveDurationMax = 4.0f;           // 이동 지속 시간 최대 (직렬화)
        
        float m_raycastDetectionRange = 20.0f;    // 레이캐스트 감지 거리 (직렬화)
        
        // ─────────────────────────────────────────────
        // 플레이어 무시 시스템 (EngageMove 충돌 후)
        // ─────────────────────────────────────────────
        int m_playerIgnoreCount = 0;              // 플레이어 무시 횟수 (방향 변경 횟수 기준)
        int m_playerIgnoreCountMin = 1;           // 무시 횟수 최소 (직렬화)
        int m_playerIgnoreCountMax = 4;           // 무시 횟수 최대 (직렬화)
        bool m_isIgnoringPlayer = false;          // 현재 플레이어 무시 중인지
        
        // ─────────────────────────────────────────────
        // 충돌 처리용 플래그
        // ─────────────────────────────────────────────
        bool m_collisionOccurred = false;         // 충돌 발생 플래그

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 충돌 콜백 (Script.h의 가상 함수 오버라이드)
        // ─────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
        // ─────────────────────────────────────────────
        // 오버라이드 (Gray 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeBullet() override;
        void Attack(float deltaTime) override;
        
        // ─────────────────────────────────────────────
        // IdleMove 상태 오버라이드
        // ─────────────────────────────────────────────
        void ExecuteIdleMoveBehaviorNonPhysics(float deltaTime) override;
        void ExecuteIdleMoveBehaviorPhysics() override;
        
        // ─────────────────────────────────────────────
        // IdleMove 헬퍼 함수
        // ─────────────────────────────────────────────
        void InitializeIdleMove();                              // IdleMove 초기화
        void ChangeDirectionRandom();                           // 랜덤 방향 전환
        void ChangeDirectionOnCollision();                      // 충돌 시 90도 전환
        void ResetMoveDuration();                               // 이동 지속 시간 재설정
        engine::Vector3 GetDirectionVector() const;             // 현재 방향 → 벡터
        bool DetectPlayerWithRaycast();                         // 4방향 레이캐스트
        
        // ─────────────────────────────────────────────
        // 플레이어 무시 시스템
        // ─────────────────────────────────────────────
        void StartPlayerIgnore();                               // 무시 시작
        void OnDirectionChanged();                              // 방향 변경 시 호출 (무시 횟수 감소)
        
        // ─────────────────────────────────────────────
        // 상태 진입 콜백 오버라이드
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
