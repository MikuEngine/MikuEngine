#pragma once

#include "MonsterRoundType.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundRed - Red 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Red 고정
    //   - 포물선 점프 이동 (45도 고정각)
    //   - 점프 중 Environment 충돌 무시
    //   - 착지점 사전 검증 (OverlapSphere)
    // 
    // 공격 패턴 (EngageAttack):
    //   - 공격점프: 플레이어 좌표로 상방 각도 점프
    //   - 공격낙하: XZ 도달 시 수직 낙하
    //   - 공격착지: 착지 순간 4방향 총알 발사
    // ═══════════════════════════════════════════════════════════════
    
    // ─────────────────────────────────────────────
    // 공격 단계 열거형
    // ─────────────────────────────────────────────
    enum class FallAttackPhase
    {
        None,       // 공격 상태 아님
        Prepare,    // 준비 (대기 시간 + 좌표 캡처)
        Jump,       // 공격점프 (상방으로 뛰어오름)
        Fall,       // 공격낙하 (수직 낙하)
        Land        // 공격착지 (총알 발사 후 사거리 체크)
    };

    class MonsterRoundRed : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundRed, MonsterRoundType)

    private:
        // ─────────────────────────────────────────────
        // 점프 이동 설정 (인스펙터 직렬화)
        // ─────────────────────────────────────────────
        float m_maxJumpStepDistance = 10.0f;       // 점프 최대 거리 (m)
        float m_jumpPrepareTime = 0.5f;            // 점프 준비 시간 (초)
        float m_launchAngle = 45.0f;               // 점프 발사 각도 (도)
        float m_ownGravity = 9.81f;                // 자체 중력 가속도 (m/s^2)
        float m_landingCheckRadius = 0.7f;         // 착지점 체크 반경 (m)
        
        // ─────────────────────────────────────────────
        // 점프 준비 상태 (런타임)
        // ─────────────────────────────────────────────
        float m_jumpPrepareTimer = 0.0f;           // 준비 타이머
        engine::Vector3 m_targetLandingPos = engine::Vector3::Zero;  // 목표 착지점
        bool m_hasValidLandingPos = false;         // 유효한 착지점 있는지
        engine::GameObject* m_landingChecker = nullptr;  // 착지점 디버그 체커
        
        // ─────────────────────────────────────────────
        // 점프 중 상태 (런타임)
        // ─────────────────────────────────────────────
        bool m_isJumping = false;                  // 점프 중인지
        uint32_t m_originalLayer = 0;              // 원래 충돌 레이어 (복원용)
        engine::TimePoint m_jumpStartTime;         // 점프 시작 시간
        bool m_landingSignal = false;              // 착지 신호 (Update → FixedUpdate)
        
        // ─────────────────────────────────────────────
        // 착지 감지 설정 (인스펙터 직렬화)
        // ─────────────────────────────────────────────
        float m_groundY = 0.0f;                    // 지면 Y 좌표
        float m_groundYOffset = 1.5f;              // 지면으로부터 떠있는 높이 (m)
        float m_jumpCheckDelay = 0.05f;            // 점프 후 착지 체크 시작 시간 (초)
        float m_landingYThreshold = 1.7f;          // 착지 시작 높이 (Y <= 이 값이면 착지 신호)
        float m_landingThreshold = 0.005f;         // 착지 판정 쓰레스홀드 (지면과의 거리)
        
        // ─────────────────────────────────────────────
        // 공격점프 설정 (인스펙터 직렬화)
        // ─────────────────────────────────────────────
        float m_fallAtkPrepareTime = 0.3f;         // 준비 대기 시간 (0~3.0초)
        float m_fallAtkLaunchAngle = 75.0f;        // 공격점프 상방 각도 (45~85도)
        float m_fallAtkJumpSpeed = 40.0f;          // 공격점프 속도 (10~100 m/s)
        float m_fallAtkLandingThreshold = 0.1f;    // XZ 도달 판정 오프셋 (0.01~2.0 m)
        float m_fallAtkPredictOffset = 1.5f;       // 예측 공격 오프셋 (플레이어 이동 시)
        float m_fallAtkMoveThreshold = 1.0f;       // 플레이어 이동 판정 임계값 (m/s)
        float m_fallAtkInitialFallSpeed = 20.0f;   // 낙하 초기 속도 (m/s)
        float m_fallAtkTerminalFallSpeed = 50.0f;  // 낙하 종점 속도 (m/s)
        float m_fallAtkLandDelay = 1.0f;           // 착지 후딜레이 (초)
        
        // ─────────────────────────────────────────────
        // 공격점프 런타임 상태
        // ─────────────────────────────────────────────
        FallAttackPhase m_fallAtkPhase = FallAttackPhase::None;  // 현재 공격 단계
        float m_fallAtkTimer = 0.0f;               // 준비/후딜레이 타이머
        engine::Vector3 m_fallAtkStartPos = engine::Vector3::Zero;   // 공격점프 시작 위치
        engine::Vector3 m_fallAtkTargetPos = engine::Vector3::Zero;  // 공격착지점 (플레이어 XZ)
        bool m_fallAtkJumping = false;             // 공격점프 중인지
        float m_fallAtkStartY = 0.0f;              // 낙하 시작 Y 좌표 (보간 계산용)
        bool m_fallAtkLandDelayStarted = false;    // 착지 후딜레이 시작 여부
        
    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // FSM 초기화 (Red 전용 상태 추가)
        // ─────────────────────────────────────────────
        void InitializeFSM() override;
        
        // ─────────────────────────────────────────────
        // 입력 처리
        // ─────────────────────────────────────────────
        void ProcessInput() override;
        
        // ─────────────────────────────────────────────
        // 상태 진입 콜백
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 비물리
        // ─────────────────────────────────────────────
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
        void ExecuteEngageJumpReadyBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageJumpBehaviorNonPhysics(float deltaTime);
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 물리
        // ─────────────────────────────────────────────
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        void ExecuteEngageJumpReadyBehaviorPhysics();
        void ExecuteEngageJumpBehaviorPhysics();
        
        // ─────────────────────────────────────────────
        // 착지점 검증 (OverlapSphere)
        // ─────────────────────────────────────────────
        bool CheckLandingPosition(const engine::Vector3& position);
        
        // 착지점 탐색 (3단계 폴백)
        bool TryFindValidLandingPosition(
            const engine::Vector3& idealPos,
            const engine::Vector3& moveDirection,
            engine::Vector3& outLandingPos);
        
        // ─────────────────────────────────────────────
        // 점프 시작/종료
        // ─────────────────────────────────────────────
        void StartJump(const engine::Vector3& landingPos);
        void OnLanding();
        
        // ─────────────────────────────────────────────
        // Y 위치 보정 (지면 + 오프셋 유지)
        // ─────────────────────────────────────────────
        void CorrectYPosition();
        
        // ─────────────────────────────────────────────
        // 오버라이드 (Red 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeBullet() override;
        void Attack(float deltaTime) override;
        
        // 행동 제한
        bool CanMove() const override;
        bool CanAttack() const override;
        
        // ─────────────────────────────────────────────
        // 공격점프 시스템
        // ─────────────────────────────────────────────
        
        // 공격 단계별 비물리 처리 (Update에서 호출)
        void ExecuteFallAttackPrepare(float deltaTime);
        void ExecuteFallAttackJump(float deltaTime);
        void ExecuteFallAttackFall(float deltaTime);
        void ExecuteFallAttackLand(float deltaTime);
        
        // 공격 단계별 물리 처리 (FixedUpdate에서 호출)
        void ExecuteFallAttackPreparePhysics();
        void ExecuteFallAttackJumpPhysics();
        void ExecuteFallAttackFallPhysics();
        void ExecuteFallAttackLandPhysics();
        
        // 공격점프 시작/종료
        void CaptureAttackLandingPosition();   // 플레이어 좌표 캡처
        void StartFallAttackJump();            // 공격점프 시작
        void OnFallAttackLanding();            // 공격착지 처리
        
        // 판정 함수
        bool HasReachedAttackLandingXZ() const;   // XZ 도달 판정
        bool CheckFallAttackLanding() const;     // 착지 판정
        
        // 총알 발사
        void FireFallAttackBullets();          // 4방향 총알 발사

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
