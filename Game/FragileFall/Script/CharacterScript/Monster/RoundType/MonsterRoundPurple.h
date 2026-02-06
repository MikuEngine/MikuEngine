#pragma once

#include "MonsterRoundType.h"

namespace engine
{
    struct CollisionInfo;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundPurple - Purple 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Purple 고정
    //   - 대각선 4방향(X자) 등속 운동 (Green과 동일)
    //   - 충돌 시 정반사 방향으로 90도 꺾임
    //   - 접촉 데미지만 존재 (총알 발사 없음)
    //   - HP 0이 되면 사망 분열 (2개로 분열)
    //   - 최대 3회 분열 (1 → 2 → 4 → 8)
    //   - 분열 시 HP/MaxHP 절반, 스케일 프리셋 적용
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundPurple : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundPurple, MonsterRoundType)

    public:
        // ─────────────────────────────────────────────
        // 스케일 프리셋 (분열 횟수별)
        // ─────────────────────────────────────────────
        static constexpr float kScalePresets[4] = { 1.0f, 0.75f, 0.5f, 0.35f };
        static constexpr int kMaxSplitCount = 3;
        
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
        engine::Vector3 m_moveDirectionVector = engine::Vector3(1.0f, 0.0f, 1.0f);
        
        // ─────────────────────────────────────────────
        // 플레이어 데미지 쿨다운
        // ─────────────────────────────────────────────
        float m_damageCooldown = 1.0f;              // 데미지 쿨다운 시간 (초)
        engine::TimePoint m_lastDamageTime;         // 마지막 데미지 준 시간
        
        // ─────────────────────────────────────────────
        // 분열 관련
        // ─────────────────────────────────────────────
        int m_splitCount = 0;                       // 현재 분열 횟수 (0, 1, 2, 3)
        std::string m_prefabName = "Monster_RoundType_Purple";  // 자기 자신 프리팹 이름
        bool m_skipIdleWait = false;                // 분열 후 Idle 대기 스킵 여부
        
        // ─────────────────────────────────────────────
        // 원본 레이어 (복원용)
        // ─────────────────────────────────────────────
        uint32_t m_originalLayer = 0;

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 충돌 콜백 (Script.h의 가상 함수 오버라이드)
        // ─────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        
        // ─────────────────────────────────────────────
        // 오버라이드 (Purple 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeFSM() override;      // Purple 전용 FSM
        void InitializeBullet() override;   // 총알 없음
        void ProcessInput() override;       // 플레이어 감지만
        
        // ─────────────────────────────────────────────
        // 상태별 행동 오버라이드
        // ─────────────────────────────────────────────
        void ExecuteEngageMoveBehaviorPhysics() override;  // 대각선 등속 이동
        
        // ─────────────────────────────────────────────
        // 공격 (접촉 데미지만, 총알 없음)
        // ─────────────────────────────────────────────
        void Attack(float deltaTime) override;  // 빈 구현
        
        // ─────────────────────────────────────────────
        // 행동 제한 오버라이드
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;
        
        // ─────────────────────────────────────────────
        // 상태 진입 콜백 오버라이드
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;
        
        // ─────────────────────────────────────────────
        // 체력 관리 오버라이드 (분열 로직)
        // ─────────────────────────────────────────────
        void CheckHealth() override;
        
        // ─────────────────────────────────────────────
        // 대각선 이동 헬퍼 함수
        // ─────────────────────────────────────────────
        void SetRandomDiagonalDirection();
        engine::Vector3 GetDiagonalDirectionVector(DiagonalDirection dir) const;
        void UpdateDiagonalDirectionFromVector();
        
        // ─────────────────────────────────────────────
        // 반사 계산
        // ─────────────────────────────────────────────
        engine::Vector3 SnapNormalToAxis(const engine::Vector3& normal) const;
        engine::Vector3 ReflectDirection(const engine::Vector3& direction, const engine::Vector3& normal) const;
        
        // ─────────────────────────────────────────────
        // 이동 로직
        // ─────────────────────────────────────────────
        void ExecuteDiagonalMovement();
        
        // ─────────────────────────────────────────────
        // 분열 로직
        // ─────────────────────────────────────────────
        void PerformSplit();                // 사망 분열 실행
        DiagonalDirection GetOppositeDirection(DiagonalDirection dir) const;  // 반대 방향 얻기

    public:
        // ─────────────────────────────────────────────
        // 총알 발사 없음 (접촉 데미지 + 분열)
        // ─────────────────────────────────────────────
        bool HasBulletAttack() const override { return false; }
        
        // ─────────────────────────────────────────────
        // 컴포넌트 검증 (StaticMesh 사용)
        // ─────────────────────────────────────────────
        bool RequiresSkeletalAnimator() const override { return false; }
        bool RequiresBulletFactory() const override { return false; }
        bool RequiresAnimFSM() const override { return false; }
        bool RequiresPathfindingAgent() const override { return false; }
        
        // ─────────────────────────────────────────────
        // 분열 횟수 설정 (인스턴시에이트 후 외부에서 설정)
        // ─────────────────────────────────────────────
        void SetSplitCount(int count) { m_splitCount = count; }
        int GetSplitCount() const { return m_splitCount; }
        
        // ─────────────────────────────────────────────
        // 이동 방향 설정 (분열 시 외부에서 설정)
        // ─────────────────────────────────────────────
        void SetDiagonalDirection(DiagonalDirection dir);
        
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
