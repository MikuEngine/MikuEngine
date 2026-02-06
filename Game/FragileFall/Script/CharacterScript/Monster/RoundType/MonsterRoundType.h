#pragma once

#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace engine
{
    class StaticMeshRenderer;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundType - 동글 공격 몬스터 공통 프레임워크
    // 
    // 특징:
    //   - MonsterScript 상속 (공통 스탯 6개 + Fragile 부활)
    //   - 5개 색상별 클래스의 기반 클래스
    //   - AttackType::Round 고정
    //   - SkeletalMesh / StaticMesh 자동 감지 (Skeletal 우선)
    // 
    // 색상별 클래스:
    //   - MonsterRoundGray, MonsterRoundGreen, MonsterRoundBlue,
    //   - MonsterRoundRed, MonsterRoundPurple
    // ═══════════════════════════════════════════════════════════════
    // 
    // ┌─────────────────────────────────────────────────────────────┐
    // │                   자식 클래스 구현 가이드                     │
    // ├─────────────────────────────────────────────────────────────┤
    // │                                                             │
    // │  [필수 오버라이드] - 자식에서 반드시 구현해야 함              │
    // │    - InitializeFSM()      : 색상별 전용 FSM 정의             │
    // │    - ProcessInput()       : 색상별 입력/감지 로직            │
    // │    - CanMove()            : 색상별 이동 가능 상태 조건       │
    // │    - CanAttack()          : 색상별 공격 가능 상태 조건       │
    // │                                                             │
    // │  [선택적 오버라이드] - 필요한 상태만 구현                     │
    // │    - ExecuteIdleMoveBehavior*()       : IdleMove 상태       │
    // │    - ExecuteEngageMoveBehavior*()     : EngageMove 상태     │
    // │    - ExecuteEngageStopBehavior*()     : EngageStop 상태     │
    // │    - ExecuteEngageAttackBehavior*()   : EngageAttack 상태   │
    // │    - ExecuteRepositioningBehavior*()  : Repositioning 상태  │
    // │    - Attack()             : 공격 패턴 (발사 로직)            │
    // │    - InitializeBullet()   : 총알 설정                        │
    // │    - OnStateEntered()     : 상태 진입 시 추가 처리           │
    // │                             (부모 호출 권장)                 │
    // │                                                             │
    // │  [공통 사용] - 오버라이드 불필요 (부모 구현 사용)             │
    // │    - ExecuteIdleBehavior*()      : Idle 상태 (타이머)        │
    // │    - ExecuteFragileBehavior*()   : Fragile 상태 (부활)       │
    // │    - ExecuteDeadBehavior*()      : Dead 상태 (파괴)          │
    // │    - UpdateStateBasedBehavior()  : 상태 분기 (자동 호출)     │
    // │    - UpdatePhysicsStateBasedBehavior() : 물리 상태 분기      │
    // │    - OnGui(), Save(), Load()     : 직렬화                    │
    // │                                                             │
    // └─────────────────────────────────────────────────────────────┘

    // ─────────────────────────────────────────────
    // 메쉬 타입 열거형
    // ─────────────────────────────────────────────
    enum class RoundMeshType
    {
        None,       // 메쉬 렌더러 없음
        Static,     // StaticMeshRenderer 사용
        Skeletal    // SkeletalMeshRenderer 사용
    };

    class MonsterRoundType : public MonsterScript
    {
        REGISTER_SCRIPT(MonsterRoundType, MonsterScript)

    protected:
        // ─────────────────────────────────────────────
        // 메쉬 타입 (자동 감지)
        // ─────────────────────────────────────────────
        RoundMeshType m_meshType = RoundMeshType::None;
        engine::StaticMeshRenderer* m_staticMeshRenderer = nullptr;
        
        // ─────────────────────────────────────────────
        // 애니메이션 이름 (자식에서 오버라이드 가능)
        // ─────────────────────────────────────────────
        std::string m_animName_Idle = "Idle";
        std::string m_animName_IdleMove = "IdleMove";
        std::string m_animName_EngageMove = "EngageMove";
        std::string m_animName_EngageStop = "EngageStop";
        std::string m_animName_EngageAttack = "EngageAttack";
        std::string m_animName_Repositioning = "Repositioning";
        std::string m_animName_Fragile = "Fragile";
        std::string m_animName_Dead = "Dead";
        
        // ─────────────────────────────────────────────
        // Idle → 다음 상태 전이 설정
        // ─────────────────────────────────────────────
        float m_idleWaitTime = 1.0f;              // Idle 대기 시간 (초)
        float m_idleTimer = 0.0f;                 // Idle 경과 시간

        // ─────────────────────────────────────────────
        // 공격 관련
        // ─────────────────────────────────────────────
        float m_attackAnimationDuration = 1.0f;   // 공격 애니메이션 재생 시간
        float m_attackAnimationTimer = 0.0f;      // 공격 애니메이션 타이머
        
        bool m_isPlayerInDetectionRange = false;  // 플레이어가 감지 거리 안에 있는지
        bool m_canFire = false;                   // 발사 가능한지 (쿨타임 체크)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        void FixedUpdate() override;


    protected:
        // ═══════════════════════════════════════════════════════════════
        // [필수 오버라이드] - 자식에서 반드시 구현해야 함
        // ═══════════════════════════════════════════════════════════════
        
        // FSM 초기화 - 자식에서 색상별 전용 FSM 정의 필수
        // 기본 구현: 샘플 8상태 FSM (실제 사용 시 오버라이드)
        void InitializeFSM() override;
        
        // 입력 처리 - 자식에서 색상별 감지/파라미터 업데이트 필수
        // 기본 구현: 거리 기반 감지 (실제 사용 시 오버라이드)
        void ProcessInput() override;
        
        // 행동 제한 - 자식에서 색상별 상태 조건 정의 필수
        // 기본 구현: IdleMove/EngageMove에서 이동, EngageAttack에서 공격
        bool CanMove() const override;
        bool CanAttack() const override;

        // ═══════════════════════════════════════════════════════════════
        // [선택적 오버라이드] - 필요한 상태만 구현
        // ═══════════════════════════════════════════════════════════════
        
        // 총알 초기화 - 공격 패턴이 있는 경우 오버라이드
        // 기본 구현: Linear 타입 총알
        void InitializeBullet() override;
        
        // 공격 - 발사 로직이 다른 경우 오버라이드
        // 기본 구현: 플레이어 방향 Linear 발사 + 애니메이션 타이머
        void Attack(float deltaTime) override;
        
        // 상태 진입 콜백 - 추가 처리 필요 시 오버라이드 (부모 호출 권장)
        // 기본 구현: 상태별 파라미터 초기화, 이동 정지 등
        void OnStateEntered(const std::string& state) override;

        // ─────────────────────────────────────────────
        // 상태별 행동 - 비물리 (자식에서 필요한 것만 오버라이드)
        // 기본 구현: 빈 구현 또는 Attack 호출
        // ─────────────────────────────────────────────
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
        virtual void ExecuteIdleMoveBehaviorNonPhysics(float deltaTime);
        virtual void ExecuteEngageMoveBehaviorNonPhysics(float deltaTime);
        virtual void ExecuteEngageStopBehaviorNonPhysics(float deltaTime);
        virtual void ExecuteEngageAttackBehaviorNonPhysics(float deltaTime);
        virtual void ExecuteRepositioningBehaviorNonPhysics(float deltaTime);
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 물리 (자식에서 필요한 것만 오버라이드)
        // 기본 구현: 빈 구현 또는 MoveTowardsPlayer/StopAllMovement
        // ─────────────────────────────────────────────
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        virtual void ExecuteIdleMoveBehaviorPhysics();
        virtual void ExecuteEngageMoveBehaviorPhysics();
        virtual void ExecuteEngageStopBehaviorPhysics();
        virtual void ExecuteEngageAttackBehaviorPhysics();
        virtual void ExecuteRepositioningBehaviorPhysics();

        // ═══════════════════════════════════════════════════════════════
        // [공통 사용] - 오버라이드 불필요
        // ═══════════════════════════════════════════════════════════════
        
        // AnimFSM 초기화 (LogicFSM 상태 → 애니메이션 매핑)
        void InitializeAnimFSM() override;
        void InitializeAnimations() override;
        
        // Idle, Fragile, Dead 상태 - 부모 로직 그대로 사용
        void ExecuteIdleBehaviorNonPhysics() override;
        void ExecuteFragileBehaviorNonPhysics() override;
        void ExecuteDeadBehaviorNonPhysics() override;
        void ExecuteIdleBehaviorPhysics() override;
        void ExecuteFragileBehaviorPhysics() override;
        void ExecuteDeadBehaviorPhysics() override;
        
        // ─────────────────────────────────────────────
        // 헬퍼 함수
        // ─────────────────────────────────────────────
        bool IsPlayerInDetectionRange() const;
        
        // ─────────────────────────────────────────────
        // 메쉬 타입 감지
        // ─────────────────────────────────────────────
        void DetectMeshType();
        bool HasAnimation() const { return m_skeletalAnimator != nullptr && m_animFSM != nullptr; }
        RoundMeshType GetMeshType() const { return m_meshType; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
