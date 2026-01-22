#include "GamePCH.h"
#include "BaseControllerScript.h"

#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void BaseControllerScript::Awake()
    {
        CacheComponents();
    }

    void BaseControllerScript::Start()
    {
        RegisterFSMCallbacks();
    }

    void BaseControllerScript::Update()
    {
        // 1. 입력 처리 (자식에서 오버라이드)
        ProcessInput();
        
        // 2. FSM 컴포넌트 업데이트
        if (m_logicFSM)
        {
            m_logicFSM->UpdateFSM();
        }
        if (m_animFSM)
        {
            m_animFSM->UpdateFSM();
        }
        
        // 3. 게임 로직 업데이트 (자식에서 오버라이드)
        UpdateGameLogic();
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 캐싱
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::CacheComponents()
    {
        // FSM 컴포넌트 찾기
        CacheFSMComponents();
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 컴포넌트 찾기
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::CacheFSMComponents()
    {
        // GetGameObject()가 유효한지 확인
        if (!GetGameObject())
        {
            return;
        }
        
        m_logicFSM = GetGameObject()->GetComponent<engine::LogicFSM>();
        m_animFSM = GetGameObject()->GetComponent<engine::AnimFSM>();
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 콜백 등록
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::RegisterFSMCallbacks()
    {
        if (!m_logicFSM) return;
        
        // 상태 변경 콜백
        m_logicFSM->RegisterStateChangeCallback([this](const std::string& oldState, const std::string& newState)
        {
            OnStateChanged(oldState, newState);
        });
        
        // 상태 진입 콜백
        m_logicFSM->RegisterStateEnterCallback([this](const std::string& state)
        {
            OnStateEntered(state);
        });
        
        // 상태 종료 콜백
        m_logicFSM->RegisterStateExitCallback([this](const std::string& state)
        {
            OnStateExited(state);
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화 헬퍼 함수들
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::AddFSMState(const std::string& stateName, bool isDefault)
    {
        if (!m_logicFSM) return;

        engine::FSMState state;
        state.name = stateName;
        state.isDefault = isDefault;
        m_logicFSM->AddState(state);
    }

    void BaseControllerScript::AddFSMTransition(
        const std::string& fromState,
        const std::string& toState,
        const std::string& parameterName,
        FSMTransitionType conditionType)
    {
        if (!m_logicFSM) return;

        engine::FSMTransition transition;
        transition.fromState = fromState;
        transition.toState = toState;
        transition.conditionParameter = parameterName;
        transition.conditionType = conditionType;
        
        m_logicFSM->AddTransition(fromState, transition);
    }
}
