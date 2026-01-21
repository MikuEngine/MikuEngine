#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class Rigidbody;
}

namespace game
{
    class InputBinding;
    class CharacterAnimationFSM;

    // 캐릭터 로직 상태 (기본)
    enum class CharacterState
    {
        Idle,       // 대기
        Walk,       // 걷기
        Attack,     // 공격
        
        // === 확장용 상태 (주석 해제하여 사용) ===
        // Run,        // 달리기
        // Jump,       // 점프
        // Fall,       // 낙하
        // Hit,        // 피격
        // Dead,       // 사망
        
        Count
    };

    // 상태 전이 시 전달되는 컨텍스트 정보
    struct StateContext
    {
        CharacterState previousState = CharacterState::Idle;
        CharacterState currentState = CharacterState::Idle;
        
        // 이동 관련
        engine::Vector3 moveDirection = engine::Vector3::Zero;
        float moveSpeed = 0.0f;
        
        // === 확장용 컨텍스트 (주석 해제하여 사용) ===
        // bool isGrounded = true;
        // int comboCount = 0;
        // engine::Vector3 hitDirection = engine::Vector3::Zero;
        // float hitDamage = 0.0f;
    };

    // 로직 FSM 이벤트 리스너 인터페이스
    class ILogicFSMListener
    {
    public:
        virtual ~ILogicFSMListener() = default;
        virtual void OnStateEnter(const StateContext& context) = 0;
        virtual void OnStateExit(const StateContext& context) = 0;
        virtual void OnStateUpdate(const StateContext& context) = 0;
    };

    // ═══════════════════════════════════════════════════════════════
    // CharacterLogicFSM - 기본 캐릭터 로직 FSM (상속용 베이스 클래스)
    // 
    // 기본 기능: 4방향 이동, 공격
    // 상속하여 확장 가능
    // ═══════════════════════════════════════════════════════════════

    class CharacterLogicFSM :
        public engine::Script<CharacterLogicFSM>
    {
        REGISTER_COMPONENT(CharacterLogicFSM)

    protected:
        // 현재 상태
        CharacterState m_currentState = CharacterState::Idle;
        StateContext m_context;
        
        // 컴포넌트 캐싱
        InputBinding* m_inputBinding = nullptr;
        CharacterAnimationFSM* m_animFSM = nullptr;
        
        // 리스너 (AnimationFSM 등)
        std::vector<ILogicFSMListener*> m_listeners;
        
        // 이동 설정
        float m_moveSpeed = 5.0f;
        
        // 상태 타이머
        float m_stateTimer = 0.0f;
        
        // 입력 바인딩 이름 (OnGui에서 설정 가능)
        std::string m_inputMoveUp = "MoveUp";
        std::string m_inputMoveDown = "MoveDown";
        std::string m_inputMoveLeft = "MoveLeft";
        std::string m_inputMoveRight = "MoveRight";
        std::string m_inputAttack = "Attack";
        
        // === 확장용 입력 바인딩 (주석 해제하여 사용) ===
        // std::string m_inputRun = "Run";
        // std::string m_inputJump = "Jump";

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        
        // ═══════════════════════════════════════════════════════════════
        // 충돌 콜백 (Push 방식)
        // 파생 클래스에서 오버라이드하여 사용
        // ═══════════════════════════════════════════════════════════════
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        void OnCollisionStay(const engine::CollisionInfo& info) override;
        void OnCollisionExit(const engine::CollisionInfo& info) override;
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
        void OnTriggerStay(const engine::CollisionInfo& info) override;
        void OnTriggerExit(const engine::CollisionInfo& info) override;

        // 상태 변경
        virtual void ChangeState(CharacterState newState);
        CharacterState GetCurrentState() const { return m_currentState; }
        const StateContext& GetContext() const { return m_context; }
        
        // 컨텍스트 직접 수정 (외부에서 정보 주입용)
        StateContext& GetContextMutable() { return m_context; }
        
        // 리스너 등록/해제
        void AddListener(ILogicFSMListener* listener);
        void RemoveListener(ILogicFSMListener* listener);
        
        // 애니메이션 FSM에서 호출 (애니메이션 종료 등)
        virtual void OnAnimationFinished(CharacterState finishedState);
        
        // === 확장용 외부 이벤트 (주석 해제하여 사용) ===
        // virtual void OnHit(const engine::Vector3& hitDirection, float damage);
        // virtual void OnDeath();
        // virtual void OnGrounded(bool isGrounded);
        // virtual void OnAnimationEvent(const std::string& eventName);
        
        // 설정 접근자
        float GetMoveSpeed() const { return m_moveSpeed; }
        void SetMoveSpeed(float speed) { m_moveSpeed = speed; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;

    protected:
        virtual void CacheComponents();
        virtual void ProcessInput();
        virtual void UpdateState();
        
        // 상태별 Enter/Exit/Update (가상 함수로 오버라이드 가능)
        virtual void OnEnterIdle();
        virtual void OnEnterWalk();
        virtual void OnEnterAttack();
        
        virtual void OnExitIdle();
        virtual void OnExitWalk();
        virtual void OnExitAttack();
        
        virtual void UpdateIdle();
        virtual void UpdateWalk();
        virtual void UpdateAttack();
        
        // === 확장용 상태 함수 (주석 해제하여 사용) ===
        // virtual void OnEnterRun();
        // virtual void OnEnterJump();
        // virtual void OnEnterFall();
        // virtual void OnEnterHit();
        // virtual void OnEnterDead();
        // virtual void OnExitRun();
        // virtual void OnExitJump();
        // virtual void OnExitFall();
        // virtual void OnExitHit();
        // virtual void OnExitDead();
        // virtual void UpdateRun();
        // virtual void UpdateJump();
        // virtual void UpdateFall();
        // virtual void UpdateHit();
        // virtual void UpdateDead();
        
        // 유틸리티
        engine::Vector3 GetMoveInputDirection() const;
        bool IsMoving() const;
        virtual bool IsAttackPressed() const;
        
        // === 확장용 유틸리티 (주석 해제하여 사용) ===
        // bool IsRunning() const;
        // bool IsJumpPressed() const;
        
        void NotifyListenersEnter();
        void NotifyListenersExit();
        void NotifyListenersUpdate();
    };
    
    // 상태 이름 문자열 변환
    const char* CharacterStateToString(CharacterState state);
}
