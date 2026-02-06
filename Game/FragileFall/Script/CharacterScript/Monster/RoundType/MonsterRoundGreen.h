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
    //   - EngageMove/EngageAttack 중 플레이어 사거리 내 + 쿨타임 완료 시 공격
    //   - EngageAttack에서도 이동/반사 로직 동일하게 유지
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundGreen : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundGreen, MonsterRoundType)

    public:
        // ─────────────────────────────────────────────
        // Green 전용 기본값 (변수 선언과 동시에 초기화 대체)
        // 씬 파일에 값이 없을 때 Load()에서 사용
        // ─────────────────────────────────────────────
        static constexpr float kDefaultFireRate = 5.0f;
        static constexpr float kDefaultDamageCooldown = 1.0f;
        
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
        
        // ─────────────────────────────────────────────
        // 컴포넌트 검증 (StaticMesh 사용)
        // ─────────────────────────────────────────────
        bool RequiresSkeletalAnimator() const override { return false; }
        bool RequiresAnimFSM() const override { return false; }
        bool RequiresPathfindingAgent() const override { return false; }

    protected:
        // ─────────────────────────────────────────────
        // 충돌 콜백 (Script.h의 가상 함수 오버라이드)
        // ─────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
        // ─────────────────────────────────────────────
        // 오버라이드 (Green 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeFSM() override;      // Green 전용 FSM (Idle, EngageMove, EngageAttack, Fragile, Dead)
        void InitializeBullet() override;   // 총알 초기화
        void ProcessInput() override;       // 플레이어 감지, 사거리/쿨타임 체크
        
        // ─────────────────────────────────────────────
        // 상태별 행동 오버라이드
        // ─────────────────────────────────────────────
        void ExecuteEngageMoveBehaviorPhysics() override;       // 대각선 등속 이동
        void ExecuteEngageAttackBehaviorNonPhysics(float deltaTime) override;  // Attack 호출
        void ExecuteEngageAttackBehaviorPhysics() override;     // 이동 유지 (EngageMove와 동일)
        
        // ─────────────────────────────────────────────
        // 공격
        // ─────────────────────────────────────────────
        void Attack(float deltaTime) override;  // 플레이어 방향 발사
        
        // ─────────────────────────────────────────────
        // 행동 제한 오버라이드
        // ─────────────────────────────────────────────
        bool CanMove() const override;      // EngageMove, EngageAttack 모두 true
        bool CanAttack() const override;    // EngageAttack일 때 true
        
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
        
        // ─────────────────────────────────────────────
        // 이동 로직 (EngageMove, EngageAttack 공용)
        // ─────────────────────────────────────────────
        void ExecuteDiagonalMovement();     // 대각선 이동 실행

    public:
        // ─────────────────────────────────────────────
        // Parabolic 타입 여부 (Green = 항상 true)
        // ─────────────────────────────────────────────
        bool IsParabolicBullet() const override { return true; }
        
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
