#include "GamePCH.h"
#include "BaseControllerScript.h"

#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
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
        // 1. 입력 처리 → FSM 파라미터 설정
        ProcessInput();
        
        // 2. FSM 상태 전이 처리
        if (m_logicFSM) m_logicFSM->UpdateFSM();
        if (m_animFSM)  m_animFSM->UpdateFSM();
        
        // 3. 상태 기반 게임 로직 실행
        UpdateGameLogic();
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한 함수 (하이브리드 패턴 핵심)
    // - 기본 구현: 대부분의 상태에서 허용
    // - 자식에서 오버라이드하여 상태별 제한 정의
    // ═══════════════════════════════════════════════════════════════
    bool BaseControllerScript::CanMove() const
    {
        // 기본: Stunned, Dead 상태가 아니면 이동 가능
        // 자식에서 오버라이드하여 커스터마이징
        std::string state = GetCurrentState();
        return state != "Stunned" && state != "Dead";
    }

    bool BaseControllerScript::CanAttack() const
    {
        // 기본: Stunned, Dead, Reloading 상태가 아니면 공격 가능
        std::string state = GetCurrentState();
        return state != "Stunned" && state != "Dead" && state != "Reloading";
    }

    bool BaseControllerScript::CanInteract() const
    {
        // 기본: Stunned, Dead 상태가 아니면 상호작용 가능
        std::string state = GetCurrentState();
        return state != "Stunned" && state != "Dead";
    }

    // ═══════════════════════════════════════════════════════════════
    // 현재 상태 확인 유틸리티
    // ═══════════════════════════════════════════════════════════════
    std::string BaseControllerScript::GetCurrentState() const
    {
        return m_logicFSM ? m_logicFSM->GetCurrentState() : "";
    }

    bool BaseControllerScript::IsInState(const std::string& stateName) const
    {
        return GetCurrentState() == stateName;
    }

    bool BaseControllerScript::IsInAnyState(const std::initializer_list<std::string>& states) const
    {
        std::string current = GetCurrentState();
        for (const auto& state : states)
        {
            if (current == state) return true;
        }
        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 캐싱
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::CacheComponents()
    {
        CacheFSMComponents();
    }

    void BaseControllerScript::CacheFSMComponents()
    {
        if (!GetGameObject()) return;
        
        m_logicFSM = GetGameObject()->GetComponent<engine::LogicFSM>();
        m_animFSM = GetGameObject()->GetComponent<engine::AnimFSM>();
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 콜백 등록
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::RegisterFSMCallbacks()
    {
        if (!m_logicFSM) return;
        
        m_logicFSM->RegisterStateChangeCallback([this](const std::string& oldState, const std::string& newState)
        {
            OnStateChanged(oldState, newState);
        });
        
        m_logicFSM->RegisterStateEnterCallback([this](const std::string& state)
        {
            OnStateEntered(state);
        });
        
        m_logicFSM->RegisterStateExitCallback([this](const std::string& state)
        {
            OnStateExited(state);
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화 헬퍼
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
