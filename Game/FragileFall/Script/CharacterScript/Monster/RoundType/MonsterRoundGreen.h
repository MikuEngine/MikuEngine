#pragma once

#include "MonsterRoundType.h"

namespace engine
{
    struct CollisionInfo;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundGreen - Green 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Green 고정
    //   - 대각선 4방향(X자) 등속 운동
    //   - 충돌 시 정반사 방향으로 90도 꺾임
    //   - 충돌면 노말은 X축 또는 Z축으로 스냅
    //   - Idle → EngageMove 전이 시 대각선 4방향 중 랜덤 선택
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundGreen : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundGreen, MonsterRoundType)

    public:
        // ─────────────────────────────────────────────
        // 대각선 이동 방향 열거형 (월드 좌표 기준)
        // ─────────────────────────────────────────────
        enum class DiagonalDirection
        {
            PlusXPlusZ,   // +X, +Z (우상)
            PlusXMinusZ,  // +X, -Z (우하)
            MinusXPlusZ,  // -X, +Z (좌상)
            MinusXMinusZ  // -X, -Z (좌하)
        };

    protected:
        // ─────────────────────────────────────────────
        // 이동 방향
        // ─────────────────────────────────────────────
        DiagonalDirection m_currentDiagonalDirection = DiagonalDirection::PlusXPlusZ;
        engine::Vector3 m_moveDirectionVector = engine::Vector3(1.0f, 0.0f, 1.0f);  // 정규화는 Start에서
        
        // ─────────────────────────────────────────────
        // 플레이어 데미지 쿨다운
        // ─────────────────────────────────────────────
        float m_damageCooldown = 1.0f;              // 데미지 쿨다운 시간 (초)
        engine::TimePoint m_lastDamageTime;         // 마지막 데미지 준 시간

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 충돌 콜백 (Script.h의 가상 함수 오버라이드)
        // ─────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
        // ─────────────────────────────────────────────
        // 오버라이드 (Green 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeFSM() override;      // Green 전용 FSM (Idle, EngageMove, Fragile, Dead)
        void InitializeBullet() override;   // 총알 초기화 (사용 안함, 빈 구현)
        void ProcessInput() override;       // Green은 레이캐스트 감지 안함
        
        // ─────────────────────────────────────────────
        // 상태별 행동 오버라이드
        // ─────────────────────────────────────────────
        void ExecuteEngageMoveBehaviorPhysics() override;  // 대각선 등속 이동
        
        // ─────────────────────────────────────────────
        // 상태 진입 콜백 오버라이드
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;
        
        // ─────────────────────────────────────────────
        // 대각선 이동 헬퍼 함수
        // ─────────────────────────────────────────────
        void SetRandomDiagonalDirection();                      // 랜덤 대각선 방향 설정
        engine::Vector3 GetDiagonalDirectionVector(DiagonalDirection dir) const;  // 열거형 → 방향벡터
        void UpdateDiagonalDirectionFromVector();               // 방향벡터 → 열거형 동기화
        
        // ─────────────────────────────────────────────
        // 반사 계산
        // ─────────────────────────────────────────────
        engine::Vector3 SnapNormalToAxis(const engine::Vector3& normal) const;    // 노말을 X/Z축으로 스냅
        engine::Vector3 ReflectDirection(const engine::Vector3& direction, const engine::Vector3& normal) const;  // 정반사

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
