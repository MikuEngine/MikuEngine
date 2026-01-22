#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class Rigidbody;
}

namespace game
{
    class CharacterAnimationFSM;

    // ═══════════════════════════════════════════════════════════════
    // CharacterState - 캐릭터 로직 상태
    // ═══════════════════════════════════════════════════════════════
    enum class CharacterState
    {
        Idle,       // 대기
        Walk,       // 걷기
        Attack,     // 공격
        Test1,      // 테스트 1
        Test2,      // 테스트 2
        Test3,      // 테스트 3
        Test4,      // 테스트 4
        
        Count
    };

    // ═══════════════════════════════════════════════════════════════
    // StateContext - 상태 전이 시 전달되는 컨텍스트 정보
    // ═══════════════════════════════════════════════════════════════
    struct StateContext
    {
        CharacterState previousState = CharacterState::Idle;
        CharacterState currentState = CharacterState::Idle;
        
        // 이동 관련
        engine::Vector3 moveDirection = engine::Vector3::Zero;
        float moveSpeed = 0.0f;
        
        // 입력 플래그 (ProcessInput에서 설정, UpdateState에서 사용)
        bool attackPressed = false;     // 공격 버튼 눌림 (1회)
        bool attackHeld = false;        // 공격 버튼 홀드 (연속)
        bool interactPressed = false;   // 상호작용 버튼
        
        // 매 프레임 입력 플래그 리셋
        void ResetInputFlags()
        {
            attackPressed = false;
            attackHeld = false;
            interactPressed = false;
        }
    };

    // ═══════════════════════════════════════════════════════════════
    // ILogicFSMListener - 로직 FSM 이벤트 리스너 인터페이스
    // ═══════════════════════════════════════════════════════════════
    class ILogicFSMListener
    {
    public:
        virtual ~ILogicFSMListener() = default;
        virtual void OnStateEnter(const StateContext& context) = 0;
        virtual void OnStateExit(const StateContext& context) = 0;
        virtual void OnStateUpdate(const StateContext& context) = 0;
    };

    // ═══════════════════════════════════════════════════════════════
    // CharacterLogicFSM - 캐릭터 로직 FSM 베이스 클래스
    // 
    // 자식 클래스에서 구현/오버라이드 가능:
    //   - ProcessInput(): 입력 처리
    //   - UpdateState(): 상태별 업데이트 로직
    //   - ChangeState(): 상태 변경 시 추가 처리
    //   - OnAnimationFinished(): 애니메이션 종료 콜백
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
        CharacterAnimationFSM* m_animFSM = nullptr;
        
        // 리스너 (AnimationFSM 등)
        std::vector<ILogicFSMListener*> m_listeners;
        
        // 이동 설정
        float m_moveSpeed = 5.0f;
        
        // 상태 타이머
        float m_stateTimer = 0.0f;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        
        // 충돌 콜백 (자식에서 오버라이드)
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        void OnCollisionStay(const engine::CollisionInfo& info) override;
        void OnCollisionExit(const engine::CollisionInfo& info) override;
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
        void OnTriggerStay(const engine::CollisionInfo& info) override;
        void OnTriggerExit(const engine::CollisionInfo& info) override;

        // 상태 변경 (자식에서 오버라이드 가능)
        virtual void ChangeState(CharacterState newState);
        
        // 상태 조회
        CharacterState GetCurrentState() const { return m_currentState; }
        const StateContext& GetContext() const { return m_context; }
        StateContext& GetContextMutable() { return m_context; }
        
        // 리스너 등록/해제
        void AddListener(ILogicFSMListener* listener);
        void RemoveListener(ILogicFSMListener* listener);
        
        // 애니메이션 FSM에서 호출 (자식에서 오버라이드 가능)
        virtual void OnAnimationFinished(CharacterState finishedState);
        
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
        
        // 상태별 Enter/Exit/Update (자식에서 오버라이드 가능)
        virtual void OnEnterState(CharacterState state);
        virtual void OnExitState(CharacterState state);
        virtual void UpdateCurrentState();
        
        // 유틸리티 (context 기반)
        bool IsMoving() const;
        
        // 리스너 알림
        void NotifyListenersEnter();
        void NotifyListenersExit();
        void NotifyListenersUpdate();
    };
    
    // 상태 이름 문자열 변환
    const char* CharacterStateToString(CharacterState state);
}
