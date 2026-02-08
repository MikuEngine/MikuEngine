#pragma once

#include "MonsterRoundType.h"

namespace engine
{
    struct CollisionInfo;
}

namespace game
{
    class CornerTrigger;
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
        
        // ─────────────────────────────────────────────
        // Wall/SubWall 충돌 쿨다운
        // ─────────────────────────────────────────────
        float m_wallCollisionCooldown = 0.1f;              // Wall 계열 충돌 쿨다운 시간 (직렬화)
        engine::TimePoint m_lastWallCollisionTime;         // 마지막 Wall 계열 충돌 시간
        std::string m_lastCollisionWallName;               // 마지막으로 방향전환한 Wall/SubWall 오브젝트 이름
        
        // ─────────────────────────────────────────────
        // EngageMove용 - 감지된 방향 저장
        // ─────────────────────────────────────────────
        engine::Vector3 m_engageDirection = engine::Vector3(1.0f, 0.0f, 0.0f);  // 레이캐스트로 감지된 방향
        bool m_fromEngageCollision = false;  // EngageMove에서 충돌로 IdleMove 진입 시 true
        
        // ─────────────────────────────────────────────
        // 코너 트리거 - 맵 모서리 회전 방향 결정
        // ─────────────────────────────────────────────
        bool m_hasCornerDirection = false;          // 코너에서 확정된 방향이 있는지
        MoveDirection m_cornerDirection = MoveDirection::PlusX;  // 코너에서 확정된 이동 방향
        
        // ─────────────────────────────────────────────
        // 플레이어 데미지 쿨다운
        // ─────────────────────────────────────────────
        float m_damageCooldown = 1.0f;              // 데미지 쿨다운 시간 (초)
        engine::TimePoint m_lastDamageTime;         // 마지막 데미지 준 시간
        
        // ─────────────────────────────────────────────
        // 맵 경계 체크 및 복원 시스템
        // ─────────────────────────────────────────────
        float m_boundaryMinX = -50.0f;                             // 맵 경계 최소 X (직렬화)
        float m_boundaryMaxX = 50.0f;                              // 맵 경계 최대 X (직렬화)
        float m_boundaryMinZ = -50.0f;                             // 맵 경계 최소 Z (직렬화)
        float m_boundaryMaxZ = 50.0f;                              // 맵 경계 최대 Z (직렬화)
        float m_obstacleCheckDistance = 2.0f;                      // 방향 전환 후 장애물 체크 거리 (직렬화)
        float m_repositioningSpeed = 5.0f;                         // 복원 이동 속도 (직렬화)
        float m_repositioningDuration = 2.0f;                      // 복원 최대 지속 시간 (직렬화)
        float m_repositioningTimer = 0.0f;                         // 복원 타이머
        engine::Vector3 m_repositioningDirection = engine::Vector3::Zero;  // 복원 방향

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 충돌/트리거 콜백 (Script.h의 가상 함수 오버라이드)
        // ─────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
        
        // ─────────────────────────────────────────────
        // 오버라이드 (Gray 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeFSM() override;  // Gray 전용 FSM (공격 상태 없음)
        void ProcessInput() override;   // Gray는 거리 기반 감지 사용 안함
        
        // ─────────────────────────────────────────────
        // 상태별 행동 오버라이드
        // ─────────────────────────────────────────────
        void ExecuteIdleMoveBehaviorNonPhysics(float deltaTime) override;
        void ExecuteIdleMoveBehaviorPhysics() override;
        void ExecuteEngageMoveBehaviorPhysics() override;  // Gray: 감지된 방향으로 직진
        
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
        // SubWall 방향 결정
        // ─────────────────────────────────────────────
        MoveDirection DetermineDirectionFromSubWall(const std::string& subWallName);
        
        // ─────────────────────────────────────────────
        // 맵 경계 및 장애물 체크 시스템
        // ─────────────────────────────────────────────
        bool HasObstacleAhead(float distance) const;            // 현재 방향으로 장애물 확인
        bool TryFindSafeDirection();                            // 안전한 방향 찾기
        bool IsOutOfBounds() const;                             // 맵 밖으로 나갔는지 확인
        void StartRepositioning();                              // 복원 시작 (중심으로)
        
        // ─────────────────────────────────────────────
        // Repositioning 상태 행동
        // ─────────────────────────────────────────────
        void ExecuteRepositioningBehaviorNonPhysics(float deltaTime) override;
        void ExecuteRepositioningBehaviorPhysics() override;
        
        // ─────────────────────────────────────────────
        // 상태 진입 콜백 오버라이드
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;

    public:
        // ─────────────────────────────────────────────
        // 총알 발사 없음 (접촉 데미지만)
        // ─────────────────────────────────────────────
        bool HasBulletAttack() const override { return false; }
        
        // ─────────────────────────────────────────────
        // 컴포넌트 검증 (StaticMesh 사용)
        // ─────────────────────────────────────────────
        bool RequiresSkeletalAnimator() const override { return false; }
        bool RequiresBulletFactory() const override { return false; }
        bool RequiresAnimFSM() const override { return false; }
        bool RequiresPathfindingAgent() const override { return false; }
        
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
