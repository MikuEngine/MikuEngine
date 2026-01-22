#pragma once

#include <Framework/Object/Component/Script.h>

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

    private:
        // FSM 컴포넌트 찾기
        void CacheFSMComponents();
        
        // FSM 콜백 등록
        void RegisterFSMCallbacks();
    };
}
