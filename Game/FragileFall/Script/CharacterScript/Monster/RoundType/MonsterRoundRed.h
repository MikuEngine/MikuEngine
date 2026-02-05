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
    // ═══════════════════════════════════════════════════════════════

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

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
