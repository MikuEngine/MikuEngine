#include "EnginePCH.h"
#include "Framework/Object/Component/LogicFSM.h"
#include "Framework/Object/Component/AnimFSM.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Engine/Core/System/MyTime.h>

namespace engine
{
    void LogicFSM::Initialize()
    {
        // 상태 맵 업데이트 (씬 파일에서 로드된 상태가 있을 수 있음)
        UpdateStateMap();
    }

    void LogicFSM::Awake()
    {
        // AnimFSM 찾기
        m_animFSM = GetGameObject()->GetComponent<AnimFSM>();
        if (m_animFSM)
        {
            m_animFSM->SetLogicFSM(this);
        }
        
        // 기본 상태 설정
        for (const auto& state : m_states)
        {
            if (state.isDefault)
            {
                m_currentState = state.name;
                break;
            }
        }
        
        // 기본 상태가 없으면 첫 번째 상태
        if (m_currentState.empty() && !m_states.empty())
        {
            m_currentState = m_states[0].name;
        }
        
        // 초기 상태 진입
        if (!m_currentState.empty())
        {
            for (auto& cb : m_stateEnterCallbacks)
            {
                if (cb) cb(m_currentState);
            }
        }
    }

    void LogicFSM::UpdateFSM()
    {
        m_stateTimer += Time::DeltaTime();
        
        // 전이 체크 (트리거 리셋 전에 체크해야 함)
        UpdateTransitions();
        
        // Trigger 파라미터 리셋 (한 프레임만 유효, 전이 체크 후)
        ResetTriggerParameters();
    }

    // ═══════════════════════════════════════════════════════════════
    // Parameters 설정
    // ═══════════════════════════════════════════════════════════════
    void LogicFSM::SetParameter(const std::string& name, float value)
    {
        auto& param = m_parameters[name];
        param.type = FSMParameter::Type::Float;
        param.name = name;
        param.floatValue = value;
    }

    void LogicFSM::SetParameter(const std::string& name, bool value)
    {
        auto& param = m_parameters[name];
        param.type = FSMParameter::Type::Bool;
        param.name = name;
        param.boolValue = value;
    }

    void LogicFSM::SetParameter(const std::string& name, int value)
    {
        auto& param = m_parameters[name];
        param.type = FSMParameter::Type::Int;
        param.name = name;
        param.intValue = value;
    }

    void LogicFSM::SetTrigger(const std::string& name)
    {
        auto& param = m_parameters[name];
        param.type = FSMParameter::Type::Trigger;
        param.name = name;
        param.triggerValue = true;
    }

    float LogicFSM::GetFloatParameter(const std::string& name) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == FSMParameter::Type::Float)
        {
            return it->second.floatValue;
        }
        return 0.0f;
    }

    bool LogicFSM::GetBoolParameter(const std::string& name) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == FSMParameter::Type::Bool)
        {
            return it->second.boolValue;
        }
        return false;
    }

    int LogicFSM::GetIntParameter(const std::string& name) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == FSMParameter::Type::Int)
        {
            return it->second.intValue;
        }
        return 0;
    }

    bool LogicFSM::GetTriggerParameter(const std::string& name) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == FSMParameter::Type::Trigger)
        {
            return it->second.triggerValue;
        }
        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 콜백 등록
    // ═══════════════════════════════════════════════════════════════
    void LogicFSM::RegisterStateChangeCallback(StateChangeCallback callback)
    {
        m_stateChangeCallbacks.push_back(callback);
    }

    void LogicFSM::RegisterStateEnterCallback(StateEnterCallback callback)
    {
        m_stateEnterCallbacks.push_back(callback);
    }

    void LogicFSM::RegisterStateExitCallback(StateExitCallback callback)
    {
        m_stateExitCallbacks.push_back(callback);
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 머신 구조 설정
    // ═══════════════════════════════════════════════════════════════
    void LogicFSM::AddState(const FSMState& state)
    {
        m_states.push_back(state);
        // 포인터는 나중에 UpdateStateMap()에서 업데이트
        // 여기서는 이름만 저장하여 재할당 문제 방지
    }

    void LogicFSM::UpdateStateMap()
    {
        m_stateMap.clear();
        for (auto& state : m_states)
        {
            m_stateMap[state.name] = &state;
        }
    }

    void LogicFSM::AddTransition(const std::string& fromState, const FSMTransition& transition)
    {
        auto it = m_stateMap.find(fromState);
        if (it != m_stateMap.end() && it->second)
        {
            // transition 객체를 복사하고 fromState를 설정
            FSMTransition newTransition = transition;
            newTransition.fromState = fromState;
            it->second->transitions.push_back(newTransition);
        }
    }

    void LogicFSM::SetDefaultState(const std::string& stateName)
    {
        for (auto& state : m_states)
        {
            state.isDefault = (state.name == stateName);
        }
    }

    void LogicFSM::InitializeCurrentState()
    {
        // 현재 상태가 비어있을 때만 초기화
        if (!m_currentState.empty())
        {
            return;
        }

        // 기본 상태 찾기
        for (const auto& state : m_states)
        {
            if (state.isDefault)
            {
                m_currentState = state.name;
                m_stateTimer = 0.0f;
                return;
            }
        }

        // 기본 상태가 없으면 첫 번째 상태
        if (!m_states.empty())
        {
            m_currentState = m_states[0].name;
            m_stateTimer = 0.0f;
        }
    }

    void LogicFSM::ClearStates()
    {
        m_states.clear();
        m_stateMap.clear();
        m_currentState.clear();
        m_previousState.clear();
    }

    // ═══════════════════════════════════════════════════════════════
    // AnimFSM 연동
    // ═══════════════════════════════════════════════════════════════
    void LogicFSM::SetAnimFSM(AnimFSM* animFSM)
    {
        m_animFSM = animFSM;
    }

    void LogicFSM::NotifyAnimationFinished(const std::string& animationName)
    {
        // AnimFSM에서 애니메이션 종료 알림
        // 필요시 상태 전이에 사용 가능
    }

    // ═══════════════════════════════════════════════════════════════
    // 내부 로직
    // ═══════════════════════════════════════════════════════════════
    void LogicFSM::ChangeState(const std::string& newState)
    {
        if (m_currentState == newState)
        {
            return;
        }

        // Exit 콜백
        for (auto& cb : m_stateExitCallbacks)
        {
            if (cb) cb(m_currentState);
        }

        // 상태 변경
        m_previousState = m_currentState;
        m_currentState = newState;
        m_stateTimer = 0.0f;

        // Enter 콜백
        for (auto& cb : m_stateEnterCallbacks)
        {
            if (cb) cb(m_currentState);
        }

        // Change 콜백
        for (auto& cb : m_stateChangeCallbacks)
        {
            if (cb) cb(m_previousState, m_currentState);
        }
    }

    bool LogicFSM::CheckTransition(const FSMTransition& transition) const
    {
        auto it = m_parameters.find(transition.conditionParameter);
        if (it == m_parameters.end())
        {
            return false;
        }

        const auto& param = it->second;

        switch (transition.conditionType)
        {
        case FSMTransition::ConditionType::Greater:
            if (param.type == FSMParameter::Type::Float)
                return param.floatValue > transition.floatThreshold;
            else if (param.type == FSMParameter::Type::Int)
                return param.intValue > transition.intThreshold;
            break;

        case FSMTransition::ConditionType::Less:
            if (param.type == FSMParameter::Type::Float)
                return param.floatValue < transition.floatThreshold;
            else if (param.type == FSMParameter::Type::Int)
                return param.intValue < transition.intThreshold;
            break;

        case FSMTransition::ConditionType::Equals:
            if (param.type == FSMParameter::Type::Float)
                return std::abs(param.floatValue - transition.floatThreshold) < 0.001f;
            else if (param.type == FSMParameter::Type::Int)
                return param.intValue == transition.intThreshold;
            break;

        case FSMTransition::ConditionType::NotEquals:
            if (param.type == FSMParameter::Type::Float)
                return std::abs(param.floatValue - transition.floatThreshold) >= 0.001f;
            else if (param.type == FSMParameter::Type::Int)
                return param.intValue != transition.intThreshold;
            break;

        case FSMTransition::ConditionType::BoolTrue:
            return param.type == FSMParameter::Type::Bool && param.boolValue;

        case FSMTransition::ConditionType::BoolFalse:
            return param.type == FSMParameter::Type::Bool && !param.boolValue;

        case FSMTransition::ConditionType::Trigger:
            return param.type == FSMParameter::Type::Trigger && param.triggerValue;
        }

        return false;
    }

    void LogicFSM::UpdateTransitions()
    {
        // 현재 상태가 비어있으면 전이 체크 불가
        if (m_currentState.empty())
        {
            return;
        }
        
        auto it = m_stateMap.find(m_currentState);
        if (it == m_stateMap.end() || !it->second)
        {
            return;
        }

        const auto& state = *it->second;

        // 전이 체크
        for (const auto& transition : state.transitions)
        {
            // Exit Time 체크
            if (transition.hasExitTime && m_animFSM)
            {
                float normalizedTime = m_animFSM->GetCurrentAnimationNormalizedTime();
                if (normalizedTime < transition.exitTime)
                {
                    continue;  // 아직 종료 시점이 아님
                }
            }

            // 조건 체크
            if (CheckTransition(transition))
            {
                ChangeState(transition.toState);
                return;  // 한 프레임에 하나의 전이만
            }
        }
    }

    void LogicFSM::ResetTriggerParameters()
    {
        for (auto& [name, param] : m_parameters)
        {
            if (param.type == FSMParameter::Type::Trigger)
            {
                param.triggerValue = false;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void LogicFSM::OnGui()
    {
        ImGui::Text("LogicFSM Component");
        ImGui::Text("Current State: %s", m_currentState.empty() ? "(None)" : m_currentState.c_str());
        ImGui::Text("State Timer: %.2f", m_stateTimer);

        ImGui::Separator();

        // States 표시 (에디터에서도 보이도록)
        if (ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (m_states.empty())
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "No states defined");
                ImGui::Text("(States will be initialized at runtime)");
                ImGui::Text("(Play the scene once and save to persist states)");
            }
            else
            {
                for (const auto& state : m_states)
                {
                    bool isCurrent = (state.name == m_currentState);
                    bool isDefault = state.isDefault;
                    
                    if (isCurrent)
                    {
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "> %s", state.name.c_str());
                        if (isDefault)
                        {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "(Current, Default)");
                        }
                        else
                        {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "(Current)");
                        }
                    }
                    else
                    {
                        if (isDefault)
                        {
                            ImGui::Text("  %s", state.name.c_str());
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "(Default)");
                        }
                        else
                        {
                            ImGui::Text("  %s", state.name.c_str());
                        }
                    }
                    
                    // 전이 정보 표시
                    if (!state.transitions.empty())
                    {
                        ImGui::Indent();
                        for (const auto& trans : state.transitions)
                        {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  -> %s", trans.toState.c_str());
                            
                            // 전이 조건 표시
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", trans.conditionParameter.c_str());
                        }
                        ImGui::Unindent();
                    }
                }
            }
        }

        ImGui::Separator();

        // Parameters 표시 (에디터에서도 보이도록)
        if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (m_parameters.empty())
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "No parameters defined");
                ImGui::Text("(Parameters will be set at runtime)");
            }
            else
            {
                for (const auto& [name, param] : m_parameters)
                {
                    ImGui::PushID(name.c_str());
                    
                    switch (param.type)
                    {
                    case FSMParameter::Type::Float:
                        ImGui::Text("%s (Float): %.2f", name.c_str(), param.floatValue);
                        break;
                    case FSMParameter::Type::Bool:
                        ImGui::Text("%s (Bool): %s", name.c_str(), param.boolValue ? "true" : "false");
                        break;
                    case FSMParameter::Type::Int:
                        ImGui::Text("%s (Int): %d", name.c_str(), param.intValue);
                        break;
                    case FSMParameter::Type::Trigger:
                        ImGui::Text("%s (Trigger): %s", name.c_str(), param.triggerValue ? "true" : "false");
                        break;
                    }
                    
                    ImGui::PopID();
                }
            }
        }
    }

    void LogicFSM::Save(json& j) const
    {
        Object::Save(j);
        
        j["CurrentState"] = m_currentState;
        
        // States 저장
        std::vector<json> statesJson;
        for (const auto& state : m_states)
        {
            json stateNode;
            stateNode["Name"] = state.name;
            stateNode["IsDefault"] = state.isDefault;
            
            // Transitions 저장
            std::vector<json> transitionsJson;
            for (const auto& trans : state.transitions)
            {
                json transNode;
                transNode["ToState"] = trans.toState;
                transNode["ConditionParameter"] = trans.conditionParameter;
                transNode["ConditionType"] = static_cast<int>(trans.conditionType);
                transNode["FloatThreshold"] = trans.floatThreshold;
                transNode["IntThreshold"] = trans.intThreshold;
                transNode["HasExitTime"] = trans.hasExitTime;
                transNode["ExitTime"] = trans.exitTime;
                transitionsJson.push_back(transNode);
            }
            stateNode["Transitions"] = transitionsJson;
            statesJson.push_back(stateNode);
        }
        j["States"] = statesJson;
    }

    void LogicFSM::Load(const json& j)
    {
        Object::Load(j);
        
        JsonGet(j, "CurrentState", m_currentState);
        
        // States 로드
        m_states.clear();
        m_stateMap.clear();
        
        JsonArrayForEach(j, "States", [&](const json& stateNode)
        {
            FSMState state;
            state.name = stateNode.value("Name", "");
            state.isDefault = stateNode.value("IsDefault", false);
            
            // Transitions 로드
            JsonArrayForEach(stateNode, "Transitions", [&](const json& transNode)
            {
                FSMTransition trans;
                trans.toState = transNode.value("ToState", "");
                trans.conditionParameter = transNode.value("ConditionParameter", "");
                trans.conditionType = static_cast<FSMTransition::ConditionType>(
                    transNode.value("ConditionType", 0));
                trans.floatThreshold = transNode.value("FloatThreshold", 0.0f);
                trans.intThreshold = transNode.value("IntThreshold", 0);
                trans.hasExitTime = transNode.value("HasExitTime", false);
                trans.exitTime = transNode.value("ExitTime", 0.0f);
                
                state.transitions.push_back(trans);
            });
            
            AddState(state);
        });
        
        // 모든 상태 추가 후 m_stateMap 업데이트
        // (Initialize()에서도 호출되지만, Load 직후에는 여기서 호출)
        UpdateStateMap();
    }   
}
