#include "GamePCH.h"
#include "CharacterAnimationFSM.h"

#include <Framework/Object/Component/SkeletalAnimator.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void CharacterAnimationFSM::Awake()
    {
        CacheComponents();
        SetupDefaultMappings();
    }

    void CharacterAnimationFSM::Start()
    {
        // 로직 FSM에 리스너로 등록
        if (m_logicFSM)
        {
            m_logicFSM->AddListener(this);
        }
    }

    void CharacterAnimationFSM::Update()
    {
        // 베이스 클래스는 빈 구현
        // 자식 클래스에서 애니메이션 종료 체크 등 구현
    }

    void CharacterAnimationFSM::OnStateEnter(const StateContext& context)
    {
        m_currentAnimState = context.currentState;
        PlayStateAnimation(context.currentState);
    }

    void CharacterAnimationFSM::OnStateExit(const StateContext& context)
    {
        // 자식 클래스에서 필요시 오버라이드
    }

    void CharacterAnimationFSM::OnStateUpdate(const StateContext& context)
    {
        HandleConditionalTransition(context);
    }

    void CharacterAnimationFSM::HandleConditionalTransition(const StateContext& context)
    {
        // 조건부 전이: 이동 속도에 따른 Walk/Run 블렌딩 등
        UpdateMoveBlending(context);
    }

    void CharacterAnimationFSM::UpdateMoveBlending(const StateContext& context)
    {
        // 현재 Walk 상태일 때만 블렌딩 처리
        if (context.currentState != CharacterState::Walk)
        {
            return;
        }

        // 속도 기반 블렌딩 (필요시 자식 클래스에서 구현)
        // 예: 걷기 -> 달리기로 부드럽게 전환
        // float normalizedSpeed = context.moveSpeed / m_logicFSM->GetRunSpeed();
        // m_animator->SetBlendParameter("Speed", normalizedSpeed);
    }

    void CharacterAnimationFSM::SetStateAnimation(CharacterState state, const AnimationTransition& transition)
    {
        m_stateAnimations[state] = transition;
    }

    void CharacterAnimationFSM::SetStateAnimation(CharacterState state, const std::string& animName,
        float crossFade, bool loop, float speed)
    {
        AnimationTransition transition;
        transition.animationName = animName;
        transition.crossFadeDuration = crossFade;
        transition.loop = loop;
        transition.speed = speed;
        transition.layerIndex = m_baseLayerIndex;
        
        m_stateAnimations[state] = transition;
    }

    void CharacterAnimationFSM::SetUpperBodyLayer(bool enabled, int layerIndex)
    {
        m_useUpperBodyLayer = enabled;
        m_upperBodyLayerIndex = layerIndex;
    }

    void CharacterAnimationFSM::SetUpperBodyWeight(float weight)
    {
        m_upperBodyWeight = weight;
        if (m_animator && m_useUpperBodyLayer)
        {
            m_animator->SetLayerWeight(m_upperBodyLayerIndex, weight);
        }
    }

    void CharacterAnimationFSM::PlayAnimation(const std::string& animName, float crossFade,
        bool loop, int layerIndex, float speed)
    {
        if (m_animator)
        {
            m_animator->PlayCrossFade(animName, crossFade, loop, layerIndex, speed);
        }
    }

    void CharacterAnimationFSM::NotifyAnimationFinished(CharacterState state)
    {
        if (m_logicFSM)
        {
            m_logicFSM->OnAnimationFinished(state);
        }
    }

    void CharacterAnimationFSM::CacheComponents()
    {
        m_animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        m_logicFSM = GetGameObject()->GetComponent<CharacterLogicFSM>();
    }

    void CharacterAnimationFSM::SetupDefaultMappings()
    {
        // 베이스 클래스는 기본 매핑만 설정
        // 자식 클래스에서 오버라이드하여 실제 애니메이션 이름 설정
        SetStateAnimation(CharacterState::Idle, "Idle", 0.2f, true);
        SetStateAnimation(CharacterState::Walk, "Walk", 0.2f, true);
        SetStateAnimation(CharacterState::Attack, "Attack", 0.1f, false);
    }

    void CharacterAnimationFSM::PlayStateAnimation(CharacterState state)
    {
        if (!m_animator)
        {
            return;
        }

        auto it = m_stateAnimations.find(state);
        if (it != m_stateAnimations.end())
        {
            const auto& transition = it->second;
            m_animator->PlayCrossFade(
                transition.animationName,
                transition.crossFadeDuration,
                transition.loop,
                transition.layerIndex,
                transition.speed
            );
        }
    }

    void CharacterAnimationFSM::OnGui()
    {
        ImGui::Text("Current Anim State: %s", CharacterStateToString(m_currentAnimState));
        
        ImGui::Separator();
        
        // 상체 분리 설정
        if (ImGui::CollapsingHeader("Upper Body Layer"))
        {
            ImGui::Checkbox("Use Upper Body Layer", &m_useUpperBodyLayer);
            ImGui::DragInt("Upper Body Layer Index", &m_upperBodyLayerIndex, 1, 0, 10);
            if (ImGui::DragFloat("Upper Body Weight", &m_upperBodyWeight, 0.01f, 0.0f, 1.0f))
            {
                SetUpperBodyWeight(m_upperBodyWeight);
            }
        }
        
        // 상태별 애니메이션 매핑
        if (ImGui::CollapsingHeader("State Animations", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (int i = 0; i < static_cast<int>(CharacterState::Count); ++i)
            {
                CharacterState state = static_cast<CharacterState>(i);
                const char* stateName = CharacterStateToString(state);
                
                ImGui::PushID(i);
                
                auto it = m_stateAnimations.find(state);
                if (it != m_stateAnimations.end())
                {
                    char animBuf[64];
                    strcpy_s(animBuf, it->second.animationName.c_str());
                    
                    ImGui::SetNextItemWidth(80);
                    ImGui::LabelText("##State", "%s", stateName);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120);
                    if (ImGui::InputText("##Anim", animBuf, 64))
                    {
                        it->second.animationName = animBuf;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(60);
                    ImGui::DragFloat("##CrossFade", &it->second.crossFadeDuration, 0.01f, 0.0f, 2.0f, "%.2f");
                    ImGui::SameLine();
                    ImGui::Checkbox("##Loop", &it->second.loop);
                }
                else
                {
                    ImGui::Text("%s: (not set)", stateName);
                }
                
                ImGui::PopID();
            }
        }
    }

    void CharacterAnimationFSM::Save(engine::json& j) const
    {
        Object::Save(j);
        
        j["UseUpperBodyLayer"] = m_useUpperBodyLayer;
        j["BaseLayerIndex"] = m_baseLayerIndex;
        j["UpperBodyLayerIndex"] = m_upperBodyLayerIndex;
        j["UpperBodyWeight"] = m_upperBodyWeight;
        j["DefaultCrossFade"] = m_defaultCrossFade;
        j["MoveBlendThreshold"] = m_moveBlendThreshold;
        
        // 상태별 애니메이션 매핑 저장
        std::vector<engine::json> mappings;
        for (const auto& [state, transition] : m_stateAnimations)
        {
            engine::json node;
            node["State"] = static_cast<int>(state);
            node["AnimationName"] = transition.animationName;
            node["CrossFadeDuration"] = transition.crossFadeDuration;
            node["Loop"] = transition.loop;
            node["LayerIndex"] = transition.layerIndex;
            node["Speed"] = transition.speed;
            mappings.push_back(node);
        }
        j["StateAnimations"] = mappings;
    }

    void CharacterAnimationFSM::Load(const engine::json& j)
    {
        Object::Load(j);
        
        engine::JsonGet(j, "UseUpperBodyLayer", m_useUpperBodyLayer);
        engine::JsonGet(j, "BaseLayerIndex", m_baseLayerIndex);
        engine::JsonGet(j, "UpperBodyLayerIndex", m_upperBodyLayerIndex);
        engine::JsonGet(j, "UpperBodyWeight", m_upperBodyWeight);
        engine::JsonGet(j, "DefaultCrossFade", m_defaultCrossFade);
        engine::JsonGet(j, "MoveBlendThreshold", m_moveBlendThreshold);
        
        // 상태별 애니메이션 매핑 로드
        m_stateAnimations.clear();
        engine::JsonArrayForEach(j, "StateAnimations", [&](const engine::json& node)
            {
                CharacterState state = static_cast<CharacterState>(node.value("State", 0));
                AnimationTransition transition;
                transition.animationName = node.value("AnimationName", "");
                transition.crossFadeDuration = node.value("CrossFadeDuration", 0.2f);
                transition.loop = node.value("Loop", true);
                transition.layerIndex = node.value("LayerIndex", 0);
                transition.speed = node.value("Speed", 1.0f);
                m_stateAnimations[state] = transition;
            }
        );
        
        // 로드 후 기본 매핑이 없으면 설정
        if (m_stateAnimations.empty())
        {
            SetupDefaultMappings();
        }
    }

    std::string CharacterAnimationFSM::GetType() const
    {
        return "CharacterAnimationFSM";
    }
}
