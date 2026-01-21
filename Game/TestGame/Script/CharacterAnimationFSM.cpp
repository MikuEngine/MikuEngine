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
        // 로직 FSM에 리스너로 등록 (LogicFSM이 먼저 Start 될 수 있으므로 여기서도 등록)
        if (m_logicFSM)
        {
            m_logicFSM->AddListener(this);
        }
    }

    void CharacterAnimationFSM::Update()
    {
        if (!m_animator || !m_logicFSM)
        {
            return;
        }

        // 현재 애니메이션 종료 체크
        if (m_currentAnimState == CharacterState::Attack)
        {
            // 공격 애니메이션이 끝났으면 로직 FSM에 알림
            int layerIndex = m_useUpperBodyLayer ? m_upperBodyLayerIndex : m_baseLayerIndex;
            
            if (m_animator->IsFinished(layerIndex))
            {
                NotifyAnimationFinished(m_currentAnimState);
            }
        }
        
        // === 확장용: Hit 상태 처리 (예시) ===
        // if (m_currentAnimState == CharacterState::Hit)
        // {
        //     if (m_animator->IsFinished(m_baseLayerIndex))
        //     {
        //         NotifyAnimationFinished(m_currentAnimState);
        //     }
        // }
    }

    void CharacterAnimationFSM::OnStateEnter(const StateContext& context)
    {
        m_currentAnimState = context.currentState;
        PlayStateAnimation(context.currentState, context);
    }

    void CharacterAnimationFSM::OnStateExit(const StateContext& context)
    {
        // 상체 레이어 사용 시, 공격 종료 후 가중치 조절
        if (m_useUpperBodyLayer && context.currentState == CharacterState::Attack)
        {
            // 공격 종료 시 상체 레이어 가중치를 부드럽게 낮춤
            // (실제로는 코루틴이나 트윈이 필요할 수 있음)
        }
    }

    void CharacterAnimationFSM::OnStateUpdate(const StateContext& context)
    {
        HandleConditionalTransition(context);
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

    void CharacterAnimationFSM::SetComboAttackAnimations(const std::vector<std::string>& anims)
    {
        m_comboAttackAnims = anims;
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
        if (!m_animator)
        {
            return;
        }

        m_animator->PlayCrossFade(animName, crossFade, loop, layerIndex, speed);
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
        // 기본 애니메이션 매핑 (프로젝트에 맞게 수정 필요)
        SetStateAnimation(CharacterState::Idle, "Idle", 0.2f, true);
        SetStateAnimation(CharacterState::Walk, "Walk", 0.2f, true);
        SetStateAnimation(CharacterState::Attack, "Attack", 0.1f, false);
        
        // === 확장용 기본 매핑 (주석 해제하여 사용) ===
        // SetStateAnimation(CharacterState::Run, "Run", 0.2f, true);
        // SetStateAnimation(CharacterState::Jump, "Jump", 0.1f, false);
        // SetStateAnimation(CharacterState::Fall, "Fall", 0.2f, true);
        // SetStateAnimation(CharacterState::Hit, "Hit", 0.1f, false);
        // SetStateAnimation(CharacterState::Dead, "Dead", 0.2f, false);
    }

    void CharacterAnimationFSM::PlayStateAnimation(CharacterState state, const StateContext& context)
    {
        if (!m_animator)
        {
            return;
        }

        // 공격 상태는 특별 처리 (콤보)
        if (state == CharacterState::Attack)
        {
            // 콤보 카운트 사용 시 아래 주석 해제
            // std::string animName = GetAnimationForCombo(context.comboCount);
            std::string animName = GetAnimationForCombo(0);
            
            int layerIndex = m_useUpperBodyLayer ? m_upperBodyLayerIndex : m_baseLayerIndex;
            
            if (m_useUpperBodyLayer)
            {
                m_animator->SetLayerWeight(m_upperBodyLayerIndex, m_upperBodyWeight);
            }
            
            m_animator->PlayCrossFade(animName, 0.1f, false, layerIndex, 1.0f);
            return;
        }

        // 일반 상태는 매핑 테이블에서 찾아서 재생
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

    std::string CharacterAnimationFSM::GetAnimationForCombo(int comboCount) const
    {
        if (m_comboAttackAnims.empty())
        {
            // 콤보 애니메이션이 설정되지 않았으면 기본 Attack 사용
            auto it = m_stateAnimations.find(CharacterState::Attack);
            if (it != m_stateAnimations.end())
            {
                return it->second.animationName;
            }
            return "Attack";
        }

        // 콤보 카운트에 맞는 애니메이션 선택 (순환)
        int index = comboCount % static_cast<int>(m_comboAttackAnims.size());
        return m_comboAttackAnims[index];
    }

    void CharacterAnimationFSM::HandleConditionalTransition(const StateContext& context)
    {
        // 조건부 전이 예시: 이동 속도에 따른 Walk/Run 블렌딩
        UpdateMoveBlending(context);
    }

    void CharacterAnimationFSM::UpdateMoveBlending(const StateContext& context)
    {
        // 현재 Walk 상태일 때만
        if (context.currentState != CharacterState::Walk)
        {
            return;
        }

        // === 확장용: Run 상태도 포함 (주석 해제하여 사용) ===
        // if (context.currentState != CharacterState::Walk && context.currentState != CharacterState::Run)
        // {
        //     return;
        // }

        // 속도 기반 블렌딩 (필요시 구현)
        // 예: 걷기 -> 달리기로 부드럽게 전환
        // float normalizedSpeed = context.moveSpeed / m_logicFSM->GetRunSpeed();
        // m_animator->SetBlendParameter("Speed", normalizedSpeed);
    }

    void CharacterAnimationFSM::OnGui()
    {
        ImGui::Text("Current Anim State: %s", CharacterStateToString(m_currentAnimState));
        
        ImGui::Separator();
        
        // 상체 분리 설정
        if (ImGui::CollapsingHeader("Upper Body Layer", ImGuiTreeNodeFlags_DefaultOpen))
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
        
        // 콤보 공격 애니메이션
        if (ImGui::CollapsingHeader("Combo Attack Animations"))
        {
            if (ImGui::Button("Add Combo"))
            {
                m_comboAttackAnims.push_back("Attack" + std::to_string(m_comboAttackAnims.size() + 1));
            }
            
            std::string removeTarget;
            for (size_t i = 0; i < m_comboAttackAnims.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                
                char buf[64];
                strcpy_s(buf, m_comboAttackAnims[i].c_str());
                ImGui::SetNextItemWidth(120);
                if (ImGui::InputText("##Combo", buf, 64))
                {
                    m_comboAttackAnims[i] = buf;
                }
                ImGui::SameLine();
                if (ImGui::Button("X"))
                {
                    removeTarget = m_comboAttackAnims[i];
                }
                
                ImGui::PopID();
            }
            
            if (!removeTarget.empty())
            {
                auto it = std::find(m_comboAttackAnims.begin(), m_comboAttackAnims.end(), removeTarget);
                if (it != m_comboAttackAnims.end())
                {
                    m_comboAttackAnims.erase(it);
                }
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
        
        // 콤보 공격 애니메이션 저장
        j["ComboAttackAnims"] = m_comboAttackAnims;
    }

    void CharacterAnimationFSM::Load(const engine::json& j)
    {
        Object::Load(j);
        
        engine::JsonGet(j, "UseUpperBodyLayer", m_useUpperBodyLayer);
        engine::JsonGet(j, "BaseLayerIndex", m_baseLayerIndex);
        engine::JsonGet(j, "UpperBodyLayerIndex", m_upperBodyLayerIndex);
        engine::JsonGet(j, "UpperBodyWeight", m_upperBodyWeight);
        engine::JsonGet(j, "DefaultCrossFade", m_defaultCrossFade);
        
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
        
        // 콤보 공격 애니메이션 로드
        if (j.contains("ComboAttackAnims"))
        {
            m_comboAttackAnims = j["ComboAttackAnims"].get<std::vector<std::string>>();
        }
        
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
    const CharacterState& CharacterAnimationFSM::GetCharacterState() const
    {
        return m_currentAnimState;
    }
}
