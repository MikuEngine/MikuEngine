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
    // BaseControllerScript - FSM과 연동하는 컨트롤러 스크립트 템플릿
    // 
    // 사용법:
    //   1. 이 클래스를 상속받아서 자식 클래스 생성
    //   2. ProcessInput() 오버라이드하여 입력 처리
    //   3. OnStateEntered(), OnStateExited() 오버라이드하여 상태별 로직
    //   4. UpdateGameLogic() 오버라이드하여 게임 로직 처리
    // 
    // 예시:
    //   class PlayerController : public BaseControllerScript
    //   {
    //       void ProcessInput() override
    //       {
    //           // WASD 입력 처리
    //           m_logicFSM->SetParameter("MoveSpeed", speed);
    //       }
    //   };
    // ═══════════════════════════════════════════════════════════════
    class BaseControllerScript :
        public engine::Script<BaseControllerScript>
    {
        REGISTER_COMPONENT(BaseControllerScript, Script)

    protected:
        // FSM 컴포넌트 참조
        engine::LogicFSM* m_logicFSM = nullptr;
        engine::AnimFSM* m_animFSM = nullptr;

        // ─────────────────────────────────────────────
        // FSM 타입 별칭 (가독성 향상)
        // ─────────────────────────────────────────────
        using FSMTransitionType = engine::FSMTransition::ConditionType;
        
        // 전이 조건 타입 헬퍼 함수 (가독성 향상)
        static constexpr FSMTransitionType BoolTrue() { return FSMTransitionType::BoolTrue; }
        static constexpr FSMTransitionType BoolFalse() { return FSMTransitionType::BoolFalse; }
        static constexpr FSMTransitionType Trigger() { return FSMTransitionType::Trigger; }
        static constexpr FSMTransitionType Greater() { return FSMTransitionType::Greater; }
        static constexpr FSMTransitionType Less() { return FSMTransitionType::Less; }
        static constexpr FSMTransitionType Equals() { return FSMTransitionType::Equals; }
        static constexpr FSMTransitionType NotEquals() { return FSMTransitionType::NotEquals; }

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    protected:
        // ─────────────────────────────────────────────
        // 오버라이드 가능한 함수들
        // ─────────────────────────────────────────────
        
        // 입력 처리 (자식에서 오버라이드)
        virtual void ProcessInput() {}
        
        // 게임 로직 업데이트 (자식에서 오버라이드)
        virtual void UpdateGameLogic() {}
        
        // 상태 변화 콜백 (자식에서 오버라이드)
        virtual void OnStateChanged(const std::string& oldState, const std::string& newState) {}
        virtual void OnStateEntered(const std::string& state) {}
        virtual void OnStateExited(const std::string& state) {}

    protected:
        // ─────────────────────────────────────────────
        // FSM 초기화 헬퍼 함수들 (자식 클래스에서 사용)
        // ─────────────────────────────────────────────
        
        // 상태 추가 헬퍼
        void AddFSMState(const std::string& stateName, bool isDefault = false);
        
        // 전이 추가 헬퍼
        void AddFSMTransition(
            const std::string& fromState,
            const std::string& toState,
            const std::string& parameterName,
            FSMTransitionType conditionType);

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 캐싱 (자식 클래스에서 오버라이드 가능)
        // ─────────────────────────────────────────────
        
        // 모든 컴포넌트 캐싱 (자식에서 오버라이드하여 추가 컴포넌트 찾기)
        virtual void CacheComponents();
        
        // FSM 컴포넌트 찾기 (CacheComponents() 내부에서 호출)
        void CacheFSMComponents();
        
        // FSM 콜백 등록
        void RegisterFSMCallbacks();
    };
}
