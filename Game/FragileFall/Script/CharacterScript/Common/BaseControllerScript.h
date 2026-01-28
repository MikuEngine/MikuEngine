#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>

namespace engine
{
    class LogicFSM;
    class AnimFSM;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // BaseControllerScript - 하이브리드 FSM 컨트롤러
    // 
    // ┌─────────────────────────────────────────────────────────────┐
    // │ 아키텍처 설명 (Hybrid FSM Pattern)                          │
    // ├─────────────────────────────────────────────────────────────┤
    // │                                                             │
    // │  ┌─────────────────────────────────────────────────────┐   │
    // │  │ LogicFSM (고수준 상태)                               │   │
    // │  │ - 캐릭터의 현재 "상태"를 정의                         │   │
    // │  │ - 행동 가능/불가능 여부 판단 기준                     │   │
    // │  │ - AnimFSM과 동기화                                   │   │
    // │  └─────────────────────────────────────────────────────┘   │
    // │                          │                                  │
    // │                          ▼                                  │
    // │  ┌─────────────────────────────────────────────────────┐   │
    // │  │ Action Layer (함수 기반)                             │   │
    // │  │ - 실제 행동 로직 (이동, 발사 등)                      │   │
    // │  │ - CanMove(), CanAttack() 등으로 FSM 상태 확인        │   │
    // │  │ - 상태가 허용하면 행동 실행                           │   │
    // │  └─────────────────────────────────────────────────────┘   │
    // │                                                             │
    // └─────────────────────────────────────────────────────────────┘
    // 
    // 사용법:
    //   1. 이 클래스를 상속받아서 자식 클래스 생성
    //   2. InitializeFSM()에서 상태와 전이 정의
    //   3. ProcessInput()에서 입력을 FSM 파라미터로 변환
    //   4. UpdateGameLogic()에서 CanMove()/CanAttack() 확인 후 행동
    // ═══════════════════════════════════════════════════════════════
    class BaseControllerScript :
        public engine::Script<BaseControllerScript>
    {
        //REGISTER_SCRIPT(BaseControllerScript, Script)
        DEFINE_COMPONENT_TYPE(BaseControllerScript, Script)

    protected:
        // ─────────────────────────────────────────────
        // FSM 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::LogicFSM* m_logicFSM = nullptr;
        engine::AnimFSM* m_animFSM = nullptr;

        // ─────────────────────────────────────────────
        // 물리 이동 설정
        // ─────────────────────────────────────────────
        // 속도 블렌딩 계수 (0.0 = 완전 물리, 1.0 = 완전 덮어쓰기)
        // - 높을수록 즉각적인 반응, 낮을수록 물리 응답 보존
        // - 충돌 시 penetration 방지를 위해 0.5 권장
        float m_velocityBlendFactor = 0.5f;

        // ─────────────────────────────────────────────
        // FSM 타입 별칭 및 헬퍼 (가독성 향상)
        // ─────────────────────────────────────────────
        using FSMTransitionType = engine::FSMTransition::ConditionType;
        
        static constexpr FSMTransitionType BoolTrue()   { return FSMTransitionType::BoolTrue; }
        static constexpr FSMTransitionType BoolFalse()  { return FSMTransitionType::BoolFalse; }
        static constexpr FSMTransitionType Trigger()    { return FSMTransitionType::Trigger; }
        static constexpr FSMTransitionType Greater()    { return FSMTransitionType::Greater; }
        static constexpr FSMTransitionType Less()       { return FSMTransitionType::Less; }
        static constexpr FSMTransitionType Equals()     { return FSMTransitionType::Equals; }
        static constexpr FSMTransitionType NotEquals()  { return FSMTransitionType::NotEquals; }

    public:
        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Awake() override;
        void Start() override;
        void Update() override;
        void FixedUpdate() override;

    protected:
        // ─────────────────────────────────────────────
        // 물리/비물리 로직 분리
        // - UpdateGameLogic(): 비물리 로직 (타이머, UI 등) - Update()에서 호출
        // - UpdatePhysicsLogic(): 물리 로직 (이동, 회전) - FixedUpdate()에서 호출
        // ─────────────────────────────────────────────
        virtual void UpdatePhysicsLogic() {}   // 물리 기반 로직 (자식에서 오버라이드)
        // ─────────────────────────────────────────────
        // 메인 로직 (자식에서 오버라이드)
        // ─────────────────────────────────────────────
        virtual void ProcessInput() {}      // 입력 → FSM 파라미터
        virtual void UpdateGameLogic() {}   // 상태 기반 게임 로직
        
        // ─────────────────────────────────────────────
        // 상태 변화 콜백 (자식에서 오버라이드)
        // ─────────────────────────────────────────────
        virtual void OnStateChanged(const std::string& oldState, const std::string& newState) {}
        virtual void OnStateEntered(const std::string& state) {}
        virtual void OnStateExited(const std::string& state) {}

        // ─────────────────────────────────────────────
        // 행동 제한 함수 (하이브리드 패턴 핵심)
        // - 자식에서 오버라이드하여 상태별 제한 정의
        // - UpdateGameLogic()에서 행동 전에 호출
        // ─────────────────────────────────────────────
        virtual bool CanMove() const;
        virtual bool CanAttack() const;
        virtual bool CanInteract() const;

        // ─────────────────────────────────────────────
        // FSM 초기화 헬퍼 (자식에서 사용)
        // ─────────────────────────────────────────────
        void AddFSMState(const std::string& stateName, bool isDefault = false);
        
        // 단일 조건 전이
        void AddFSMTransition(
            const std::string& fromState,
            const std::string& toState,
            const std::string& parameterName,
            FSMTransitionType conditionType);
        
        // 여러 조건 전이 (가변 인자)
        template<typename... Args>
        void AddFSMTransition(
            const std::string& fromState,
            const std::string& toState,
            const std::string& param1, FSMTransitionType type1,
            const std::string& param2, FSMTransitionType type2,
            Args&&... args)
        {
            if (!m_logicFSM) return;

            engine::FSMTransition transition;
            transition.toState = toState;
            transition.conditionParameter = param1;
            transition.conditionType = type1;

            // 두 번째 조건 추가
            engine::FSMTransition::AdditionalCondition cond2;
            cond2.parameterName = param2;
            cond2.conditionType = type2;
            transition.additionalConditions.push_back(cond2);

            // 나머지 조건들 추가 (재귀적으로 처리)
            AddAdditionalConditions(transition, std::forward<Args>(args)...);

            m_logicFSM->AddTransition(fromState, transition);
        }

    private:
        // 추가 조건 재귀 처리 (베이스 케이스)
        void AddAdditionalConditions(engine::FSMTransition& transition) {}

        // 추가 조건 재귀 처리
        template<typename... Args>
        void AddAdditionalConditions(
            engine::FSMTransition& transition,
            const std::string& paramName, FSMTransitionType condType,
            Args&&... args)
        {
            engine::FSMTransition::AdditionalCondition cond;
            cond.parameterName = paramName;
            cond.conditionType = condType;
            transition.additionalConditions.push_back(cond);

            // 재귀 호출
            AddAdditionalConditions(transition, std::forward<Args>(args)...);
        }

    protected:

        // ─────────────────────────────────────────────
        // 현재 상태 확인 유틸리티
        // ─────────────────────────────────────────────
        std::string GetCurrentState() const;
        bool IsInState(const std::string& stateName) const;
        bool IsInAnyState(const std::initializer_list<std::string>& states) const;

        // ─────────────────────────────────────────────
        // 방향 유틸리티
        // - SkeletalAnimator가 FBX를 180도 회전시킴
        // - GetForwardDirection(): +Z = 모델의 실제 앞 방향
        // ─────────────────────────────────────────────
        engine::Vector3 GetForwardDirection() const;
        engine::Vector3 GetForwardDirectionReverse() const;

        // ─────────────────────────────────────────────
        // 대상(Target) 유틸리티 - 범용 함수
        // ─────────────────────────────────────────────
        float GetDistanceToTarget(engine::GameObject* target) const;
        engine::Vector3 CalculateDirectionToTarget(engine::GameObject* target) const;
        bool IsTargetInRange(engine::GameObject* target, float range) const;
        
        // ─────────────────────────────────────────────
        // 회전 유틸리티 - 범용 함수
        // ─────────────────────────────────────────────
        void RotateTowardsTarget(engine::GameObject* target, float deltaTime);
        void RotateToDirection(const engine::Vector3& targetDirection, float deltaTime);
        bool IsLookingAtTarget(engine::GameObject* target) const;
        bool IsLookingAtDirection(const engine::Vector3& targetDirection) const;
        
        void StopRotation();

        // ─────────────────────────────────────────────
        // 컴포넌트 캐싱
        // ─────────────────────────────────────────────
        virtual void CacheComponents();
        void CacheFSMComponents();
        void RegisterFSMCallbacks();
        
    private:
        // Rigidbody 캐시 (회전 함수에서 사용)
        mutable engine::Rigidbody* m_cachedRigidbody = nullptr;
    };
}
