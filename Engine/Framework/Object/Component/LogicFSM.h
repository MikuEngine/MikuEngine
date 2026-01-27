#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class SkeletalAnimator;

    // ═══════════════════════════════════════════════════════════════
    // FSMParameter - FSM 파라미터 (Script에서 설정)
    // ═══════════════════════════════════════════════════════════════
    struct FSMParameter
    {
        enum class Type
        {
            Float,
            Bool,
            Int,
            Trigger
        };
        
        Type type;
        std::string name;
        
        union
        {
            float floatValue;
            bool boolValue;
            int intValue;
        };
        
        bool triggerValue = false;  // Trigger는 별도 처리
        
        FSMParameter() : type(Type::Float), floatValue(0.0f) {}
    };

    // ═══════════════════════════════════════════════════════════════
    // FSMTransition - 상태 전이 조건 (에디터에서 설정)
    // ═══════════════════════════════════════════════════════════════
    struct FSMTransition
    {
        std::string fromState;
        std::string toState;
        
        // 전이 조건
        std::string conditionParameter;  // 파라미터 이름
        enum class ConditionType
        {
            Greater,      // Float/Int > value
            Less,         // Float/Int < value
            Equals,       // Float/Int == value
            NotEquals,    // Float/Int != value
            BoolTrue,     // Bool == true
            BoolFalse,    // Bool == false
            Trigger,       // Trigger == true
            Default         // 변수 초기화용
        };
        ConditionType conditionType = FSMTransition::ConditionType::Default;
        
        float floatThreshold = 0.0f;
        int intThreshold = 0;
        
        bool hasExitTime = false;        // 애니메이션 종료 대기
        float exitTime = 0.0f;          // 종료 시점 (0.0 ~ 1.0)
        
        // ─────────────────────────────────────────────
        // 추가 조건 (AND로 연결)
        // ─────────────────────────────────────────────
        struct AdditionalCondition
        {
            std::string parameterName;
            ConditionType conditionType = ConditionType::Default;
            float floatThreshold = 0.0f;
            int intThreshold = 0;
        };
        std::vector<AdditionalCondition> additionalConditions;
    };

    // ═══════════════════════════════════════════════════════════════
    // FSMState - FSM 상태 (에디터에서 설정)
    // ═══════════════════════════════════════════════════════════════
    struct FSMState
    {
        std::string name;
        std::vector<FSMTransition> transitions;
        bool isDefault = false;  // 초기 상태
    };

    // ═══════════════════════════════════════════════════════════════
    // LogicFSM - 로직 상태 머신 컴포넌트
    // 
    // 기능:
    //   - 에디터에서 상태와 전이 설정
    //   - Script에서 Parameters 설정
    //   - 상태 변경 이벤트 발생
    // ═══════════════════════════════════════════════════════════════
    class LogicFSM :
        public Component
    {
        REGISTER_COMPONENT(LogicFSM, Component)

    public:
        // 상태 변경 콜백 타입
        using StateChangeCallback = std::function<void(const std::string& oldState, const std::string& newState)>;
        using StateEnterCallback = std::function<void(const std::string& state)>;
        using StateExitCallback = std::function<void(const std::string& state)>;

    private:
        // 상태 머신 구조 (에디터에서 설정)
        std::vector<FSMState> m_states;
        std::unordered_map<std::string, FSMState*> m_stateMap;
        
        // 현재 상태
        std::string m_currentState;
        std::string m_previousState;
        float m_stateTimer = 0.0f;
        
        // Parameters (Script에서 설정)
        std::unordered_map<std::string, FSMParameter> m_parameters;
        
        // 콜백
        std::vector<StateChangeCallback> m_stateChangeCallbacks;
        std::vector<StateEnterCallback> m_stateEnterCallbacks;
        std::vector<StateExitCallback> m_stateExitCallbacks;
        
        // AnimFSM 연동
        class AnimFSM* m_animFSM = nullptr;

    public:
        void Initialize() override;
        void Awake() override;
        
        
        void UpdateFSM();

        // ─────────────────────────────────────────────
        // 상태 조회
        // ─────────────────────────────────────────────
        std::string GetCurrentState() const { return m_currentState; }
        std::string GetPreviousState() const { return m_previousState; }
        float GetStateTimer() const { return m_stateTimer; }
        
        // ─────────────────────────────────────────────
        // Parameters 설정 (Script에서 호출)
        // ─────────────────────────────────────────────
        void SetParameter(const std::string& name, float value);
        void SetParameter(const std::string& name, bool value);
        void SetParameter(const std::string& name, int value);
        void SetTrigger(const std::string& name);  // Trigger는 한 프레임만 true
        
        // Parameters 조회
        float GetFloatParameter(const std::string& name) const;
        bool GetBoolParameter(const std::string& name) const;
        int GetIntParameter(const std::string& name) const;
        bool GetTriggerParameter(const std::string& name) const;
        
        // ─────────────────────────────────────────────
        // 콜백 등록 (Script에서 호출)
        // ─────────────────────────────────────────────
        void RegisterStateChangeCallback(StateChangeCallback callback);
        void RegisterStateEnterCallback(StateEnterCallback callback);
        void RegisterStateExitCallback(StateExitCallback callback);
        
        // ─────────────────────────────────────────────
        // 상태 머신 구조 설정 (에디터/직렬화용)
        // ─────────────────────────────────────────────
        void AddState(const FSMState& state);
        void AddTransition(const std::string& fromState, const FSMTransition& transition);
        void SetDefaultState(const std::string& stateName);
        void ClearStates();
        void UpdateStateMap();  // 모든 상태 추가 후 m_stateMap 업데이트
        void InitializeCurrentState();  // 기본 상태로 현재 상태 초기화 (상태 추가 후 호출)
        
        // ─────────────────────────────────────────────
        // AnimFSM 연동
        // ─────────────────────────────────────────────
        void SetAnimFSM(class AnimFSM* animFSM);
        void NotifyAnimationFinished(const std::string& animationName);

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    private:
        void ChangeState(const std::string& newState);
        bool CheckTransition(const FSMTransition& transition) const;
        void UpdateTransitions();
        void ResetTriggerParameters();
    };
}
