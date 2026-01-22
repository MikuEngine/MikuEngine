#include "GamePCH.h"
#include "BaseControllerScript.h"

#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void BaseControllerScript::Awake()
    {
        CacheFSMComponents();
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
    // FSM 컴포넌트 찾기
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::CacheFSMComponents()
    {
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
}
