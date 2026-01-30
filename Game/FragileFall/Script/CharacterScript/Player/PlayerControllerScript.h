#pragma once

#include "Script/CharacterScript/Common/BaseControllerScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace engine
{
    class Rigidbody;
    class SkeletalAnimator;
}

namespace game
{
    class AimPointer;
    class BulletFactory;

    // ═══════════════════════════════════════════════════════════════
    // PlayerControllerScript - 하이브리드 FSM 기반 슈팅 플레이어
    // 
    // 아키텍처:
    //   - LogicFSM: 고수준 상태 (Idle, Walk, IdleShoot, WalkShoot)
    //   - 행동 로직: 함수 기반, CanMove()/CanAttack()으로 제한
    //   - AnimFSM: LogicFSM 상태에 따라 애니메이션 재생
    // 
    // FSM 상태:
    //   - Idle: 정지 상태
    //   - Walk: 이동 상태
    //   - IdleShoot: 정지 + 발사
    //   - WalkShoot: 이동 + 발사
    // ═══════════════════════════════════════════════════════════════
    class PlayerControllerScript : public BaseControllerScript
    {
        REGISTER_SCRIPT(PlayerControllerScript, BaseControllerScript)

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::SkeletalAnimator* m_skeletalAnimator = nullptr;
        AimPointer* m_aimPointer = nullptr;
        BulletFactory* m_bulletFactory = nullptr;

        // ─────────────────────────────────────────────
        // 이동 설정
        // ─────────────────────────────────────────────
        float m_moveSpeed = 13.0f;
        float m_rotationSpeed = 10.0f;  // 회전 속도 (rad/sec)
        // 이동 가속/감속 설정은 BaseControllerScript에서 상속
        // (m_movementAcceleration, m_movementDeceleration, m_maxSpeedBrakeFactor)

        // ─────────────────────────────────────────────
        // 대쉬 설정
        // ─────────────────────────────────────────────
        float m_dashDuration = 1.0f;                    // 대쉬 지속 시간 (초)
        float m_dashInitialSpeedMultiplier = 3.0f;      // 초기 속도 배율 (m_moveSpeed 기준)
        float m_dashCooldown = 2.0f;                    // 대쉬 쿨다운 (초)

        // ─────────────────────────────────────────────
        // 대쉬 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isDashing = false;                       // 대쉬 중 여부
        float m_dashCooldownTimer = 0.0f;               // 쿨다운 타이머
        float m_dashElapsedTime = 0.0f;                 // 대쉬 경과 시간
        engine::Vector3 m_dashDirection = engine::Vector3::Zero;  // 대쉬 방향 (시작 시 고정)

        // ─────────────────────────────────────────────
        // 대쉬 충돌 감쇠 시스템
        // - 대쉬 중 벽/적과 충돌 시 감쇠 지수를 높여 빠르게 감속
        // ─────────────────────────────────────────────
        float m_dashCollisionDecayBoost = 1.0f;         // 충돌 시 감쇠 배율 (기본 1.0)
        float m_dashCollisionDecayMultiplier = 5.0f;    // 충돌 감지 시 적용할 감쇠 배율
        float m_dashCollisionDotThreshold = 0.3f;       // 정면 충돌 판정 임계값 (0.3 ≈ 72도)

        // ─────────────────────────────────────────────
        // SphereCast 기반 지형 파고들기 방지 시스템
        // - 이동/대쉬 방향으로 SphereCast하여 Environment 레이어 감지
        // - 감지 시 해당 방향 이동 차단 또는 대쉬 속도 감소
        // ─────────────────────────────────────────────
        float m_sphereCastRadius = 1.0f;                // SphereCast 반지름
        float m_sphereCastDistance = 1.0f;              // SphereCast 감지 거리
        float m_dashWallSpeedMultiplier = 0.1f;         // 벽 감지 시 대쉬 초기 속도 배율 (0.1 = 10%)
        
        // SphereCast 런타임 상태
        bool m_environmentBlockDetected = false;        // 현재 프레임에서 Environment 감지 여부
        bool m_dashWallDetectedOnStart = false;         // 대쉬 시작 시 벽 감지 여부
        engine::Vector3 m_lastBlockNormal = engine::Vector3::Zero;  // 마지막으로 감지된 벽의 노말

        // ─────────────────────────────────────────────
        // 발사 설정 (쿨다운/타이밍은 Player가 관리)
        // ─────────────────────────────────────────────
        float m_fireRate = 0.2f;         // 발사 간격 (초)
        float m_bulletSpeed = 1.0f;     // 총알 속도
        float m_bulletLifetime = 3.0f;   // 총알 수명 (초)

        // ─────────────────────────────────────────────
        // 처형 시스템 설정
        // ─────────────────────────────────────────────
        float m_executionRange = 10.0f;  // 처형 가능 거리

        // ─────────────────────────────────────────────
        // 참조 설정
        // ─────────────────────────────────────────────
        std::string m_aimPointerObjectName = "AimPointer";  // 씬에서 찾을 AimPointer 오브젝트 이름

        // ─────────────────────────────────────────────
        // 기타 설정
        // ─────────────────────────────────────────────
        bool m_enableUpperBodyAim = true;

        // ─────────────────────────────────────────────
        // 애니메이션 설정 (Initialize에서 SkeletalAnimator에 등록할 애니메이션 이름)
        // ─────────────────────────────────────────────
        std::string m_animName_Idle = "Idle";
        std::string m_animName_WalkForward = "WalkForward";
        std::string m_animName_WalkBackward = "WalkBackward";
        std::string m_animName_Fire = "Fire";  // 발사 애니메이션 (현재 Punch 애니메이션)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_fireTimer = 0.0f;
        bool m_fsmInitialized = false;
        
        // 마지막 이동 방향 (캐릭터가 서있는 방향)
        // 초기값: -Z 방향 (아래쪽)
        engine::Vector3 m_lastMoveDirection = engine::Vector3(0.0f, 0.0f, -1.0f);

        // 캐릭터의 정면으로 사용할 벡터 == 현재 조준 방향
        // 오브젝트 트랜스폼과 관계 없음
        engine::Vector3 m_ForwardAimDir = engine::Vector3(0.0f, 0.0f, -1.0f);
        
        // 현재 캐릭터 하체 회전 각도 (라디안)
        float m_currentRotationAngle = 0.0f;
        
        // ─────────────────────────────────────────────
        // 회전 방식 선택
        // true: ForceSetRotation 기반 (직접 트랜스폼 제어, 권장)
        // false: Angular Velocity 기반 (물리 기반, 회전속도 12.0f, AngularDamping 10 권장)
        // ─────────────────────────────────────────────
        bool m_useForceSetRotation = true;
        
        // 정지 상태 목표 방향 (90도씩 양자화된 회전용)
        engine::Vector3 m_idleTargetDir = engine::Vector3(0.0f, 0.0f, 1.0f);
        bool m_idleTargetValid = false;  // 목표가 유효한지
        
        // 이동 상태 (Front/Back) - 히스테리시스용
        bool m_isWalkingBackward = false;
        
        // 이전 프레임 이동 상태 (이동 시작 감지용)
        bool m_wasMoving = false;       
       

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // BaseControllerScript 오버라이드
        // ─────────────────────────────────────────────
        void CacheComponents() override;
        void ProcessInput() override;
        void UpdateGameLogic() override;         // 비물리 로직 (타이머, 애니메이션 등)
        void UpdatePhysicsLogic() override;      // 물리 로직 (이동, 회전)
        void OnStateEntered(const std::string& state) override;

        // ─────────────────────────────────────────────
        // 행동 제한 (하이브리드 패턴)
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;

        // ─────────────────────────────────────────────
        // 충돌 콜백 (대쉬 충돌 감쇠용)
        // ─────────────────────────────────────────────
        void OnCollisionStay(const engine::CollisionInfo& info) override;

    private:
        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        void InitializeFSM();
        void InitializeAnimFSM();  // AnimFSM 상태 매핑 등록
        void InitializeAnimations();  // SkeletalAnimator에 애니메이션 등록

        // ─────────────────────────────────────────────
        // 입력 유틸리티
        // ─────────────────────────────────────────────
        engine::Vector3 GetMoveInputDirection() const;

        // ─────────────────────────────────────────────
        // 액션 함수
        // ─────────────────────────────────────────────
        
        void HandleDash();               // 대쉬 처리 (FixedUpdate에서 호출)
        void StartDash();                // 대쉬 시작
        void EndDash();                  // 대쉬 종료
        float CalculateDashSpeed() const; // 현재 대쉬 속도 계산 (지수 감쇠)
        void HandleShooting(float deltaTime);  // Update에서 호출 (DeltaTime 사용)

        // ─────────────────────────────────────────────
        // SphereCast 기반 파고들기 방지
        // ─────────────────────────────────────────────
        // 이동 방향으로 SphereCast하여 Environment 레이어 감지
        // 반환값: 벽 감지 시 true, 아니면 false
        // outNormal: 감지된 벽의 노말 (슬라이딩 계산용)
        bool CheckEnvironmentBlock(const engine::Vector3& direction, engine::Vector3& outNormal);
        
        // 벽 슬라이딩이 적용된 이동 방향 계산
        // 벽에 수직인 성분만 제거하고 평행한 성분은 유지
        engine::Vector3 CalculateSlidingDirection(const engine::Vector3& moveDir, const engine::Vector3& wallNormal);
                
        //void RotateToDirection(const engine::Vector3& targetDirection, float deltaTime);
        

        // ─────────────────────────────────────────────
        // 애니메이션 제어
        // ─────────────────────────────────────────────
        void UpdateAnimation();             
        std::string GetAnimationState() const;
        
        // ─────────────────────────────────────────────
        // 내가 직접 짜집기한 에임 추적, 회전, Forward-Back 스위칭 유틸리티
        // ─────────────────────────────────────────────
            
        //void HandleMovement(float deltaTime);
                     
        engine::Vector3 m_inputMoveDir = engine::Vector3(0.0f, 0.0f, 0.0f);
        engine::Vector3 m_playerLogicalForward;
        engine::Vector3 m_targetRotateDirLowerbodyLogical;

        void CheckForwardBack(engine::Vector3& forward, engine::Vector3& aimDir);
        bool m_isBackward = false;        

        engine::Vector3 m_prevAimDirection = engine::Vector3(0.0f, 0.0f, 1.0f);  // 이전 에임 방향 (정규화)
        engine::Vector3 m_currentAimDir = engine::Vector3(0.0f, 0.0f, 1.0f);
        engine::Vector3 m_targetAimDirection = engine::Vector3(0.0f, 0.0f, 1.0f);     

        float m_aimCrossProduct = 0.0f;       // 현재 방향과 목표 방향의 외적 값 (양수: 반시계, 음수: 시계)
        bool m_aimTrackingInitialized = false;


        // ─────────────────────────────────────────────
        // 에디터 검증
        // ─────────────────────────────────────────────
        bool ValidateComponents() const;  // 컴포넌트 유효성 검사

    public:
        // ─────────────────────────────────────────────
        // 처형 시스템 접근자
        // ─────────────────────────────────────────────
        float GetExecutionRange() const { return m_executionRange; }

        // 처형 시작 (ExecutionIndicatorManager에서 호출)
        void StartExecution(engine::GameObject* targetMonster);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
