#include "EnginePCH.h"
#include "Framework/Object/Component/AnimFSM.h"
#include "Framework/Object/Component/LogicFSM.h"
#include "Framework/Object/Component/Animator/SkeletalAnimator.h"
#include "Framework/Object/GameObject/GameObject.h"

namespace engine
{
    void AnimFSM::Initialize()
    {
        // 초기화
    }

    void AnimFSM::Awake()
    {
        // SkeletalAnimator 찾기
        m_animator = GetGameObject()->GetComponent<SkeletalAnimator>();
        
        // LogicFSM 찾기
        m_logicFSM = GetGameObject()->GetComponent<LogicFSM>();
        if (m_logicFSM)
        {
            m_logicFSM->SetAnimFSM(this);
            
            // LogicFSM 상태 변화 콜백 등록
            m_logicFSM->RegisterStateChangeCallback([this](const std::string& oldState, const std::string& newState)
            {
                OnLogicStateChanged(oldState, newState);
            });
            
            m_logicFSM->RegisterStateEnterCallback([this](const std::string& state)
            {
                OnLogicStateEntered(state);
            });
        }
    }

    void AnimFSM::UpdateFSM()
    {
        // Procedural 조준 업데이트
        UpdateProceduralAim();
    }

    // ═══════════════════════════════════════════════════════════════
    // LogicFSM 연동
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::SetLogicFSM(LogicFSM* logicFSM)
    {
        m_logicFSM = logicFSM;
    }

    void AnimFSM::OnLogicStateChanged(const std::string& oldState, const std::string& newState)
    {
        // 상태 변경 시 애니메이션 재생 (자동 연동 모드)
        // 해당 상태가 등록되어 있을 때만 재생 (스크립트에서 직접 제어하는 경우를 위해)
        if (m_states.find(newState) != m_states.end())
        {
            PlayStateAnimation(newState);
        }
    }

    void AnimFSM::OnLogicStateEntered(const std::string& state)
    {
        // 상태 진입 시 애니메이션 재생
        // 해당 상태가 등록되어 있을 때만 재생
        if (m_states.find(state) != m_states.end())
        {
            PlayStateAnimation(state);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 애니메이션 상태 등록 (통합 API)
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::AddState(const AnimState& state)
    {
        m_states[state.stateName] = state;
    }

    void AnimFSM::RemoveState(const std::string& stateName)
    {
        m_states.erase(stateName);
    }

    void AnimFSM::ClearStates()
    {
        m_states.clear();
    }

    void AnimFSM::AddDefaultState(
        const std::string& stateName,
        const std::string& animationName,
        bool loop,
        float crossFade,
        int layerIndex,
        float speed)
    {
        AnimState state;
        state.stateName = stateName;
        state.useSplitAnimation = false;
        state.animationName = animationName;
        state.loop = loop;
        state.layerIndex = layerIndex;
        state.speed = speed;
        state.crossFadeDuration = crossFade;
        
        m_states[stateName] = state;
    }

    void AnimFSM::AddSplitState(
        const std::string& stateName,
        const std::string& lowerAnim, bool lowerLoop,
        const std::string& upperAnim, bool upperLoop,
        float upperWeight,
        float crossFade)
    {
        AnimState state;
        state.stateName = stateName;
        state.useSplitAnimation = true;
        state.lowerAnimation = lowerAnim;
        state.lowerLoop = lowerLoop;
        state.upperAnimation = upperAnim;
        state.upperLoop = upperLoop;
        state.upperBodyWeight = upperWeight;
        state.crossFadeDuration = crossFade;
        
        m_states[stateName] = state;
    }

    // ═══════════════════════════════════════════════════════════════
    // 레이어 설정
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::SetLayerIndices(int baseLayer, int upperBodyLayer)
    {
        m_baseLayerIndex = baseLayer;
        m_upperBodyLayerIndex = upperBodyLayer;
    }

    // ═══════════════════════════════════════════════════════════════
    // Procedural 상체 회전
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::SetProceduralAimEnabled(bool enabled)
    {
        m_enableProceduralAim = enabled;
        
        if (!enabled && m_animator)
        {
            m_animator->SetProceduralRotation(m_spineBoneName, Quaternion::Identity);
        }
    }

    void AnimFSM::SetUpperBodyYaw(float degrees)
    {
        m_upperBodyYaw = std::clamp(degrees, -m_maxYaw, m_maxYaw);
    }

    void AnimFSM::SetUpperBodyPitch(float degrees)
    {
        m_upperBodyPitch = std::clamp(degrees, -m_maxPitch, m_maxPitch);
    }

    void AnimFSM::SetUpperBodyAim(float yawDegrees, float pitchDegrees)
    {
        m_upperBodyYaw = std::clamp(yawDegrees, -m_maxYaw, m_maxYaw);
        m_upperBodyPitch = std::clamp(pitchDegrees, -m_maxPitch, m_maxPitch);
    }

    void AnimFSM::SetAimLimits(float maxYaw, float maxPitch)
    {
        m_maxYaw = maxYaw;
        m_maxPitch = maxPitch;
    }

    void AnimFSM::SetSpineBoneName(const std::string& boneName)
    {
        m_spineBoneName = boneName;
    }

    void AnimFSM::UpdateProceduralAim()
    {
        if (!m_enableProceduralAim) return;
        
        float dt = Time::DeltaTime();
        
        // 부드러운 보간
        m_currentYaw = std::lerp(m_currentYaw, m_upperBodyYaw, m_aimLerpSpeed * dt);
        m_currentPitch = std::lerp(m_currentPitch, m_upperBodyPitch, m_aimLerpSpeed * dt);
        
        ApplyProceduralRotation();
    }

    void AnimFSM::ApplyProceduralRotation()
    {
        if (!m_animator) return;
        
        // degree → radian
        float yawRad = ToRadian(m_currentYaw);
        float pitchRad = ToRadian(m_currentPitch);
        
        // Quaternion 생성
        Quaternion yawRot = Quaternion::CreateFromAxisAngle(Vector3::UnitY, yawRad);
        Quaternion pitchRot = Quaternion::CreateFromAxisAngle(Vector3::UnitX, pitchRad);
        Quaternion finalRot = yawRot * pitchRot;
        
        // SkeletalAnimator에 적용
        m_animator->SetProceduralRotation(m_spineBoneName, finalRot);
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 조회 및 제어
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::SetAnimState(const std::string& stateName)
    {
        // 이전 상태와 같으면 스킵
        if (stateName == m_currentState) return;
        
        PlayStateAnimation(stateName);
    }

    void AnimFSM::PlayUpperBodyAnimation(const std::string& animName, bool loop)
    {
        if (!m_animator) return;
        
        // 상체 레이어 활성화 및 애니메이션 재생
        m_animator->SetLayerWeight(m_upperBodyLayerIndex, 1.0f);
        m_animator->Play(animName, loop, m_upperBodyLayerIndex, 1.0f);
    }

    float AnimFSM::GetCurrentAnimationNormalizedTime() const
    {
        if (!m_animator) return 0.0f;
        return m_animator->GetNormalizedTime(m_baseLayerIndex);
    }

    bool AnimFSM::IsAnimationFinished() const
    {
        if (!m_animator) return false;
        return m_animator->IsFinished(m_baseLayerIndex);
    }

    void AnimFSM::NotifyAnimationFinished(const std::string& animationName)
    {
        if (m_logicFSM)
        {
            m_logicFSM->NotifyAnimationFinished(animationName);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 내부 로직 - 애니메이션 재생
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::PlayStateAnimation(const std::string& stateName)
    {
        if (!m_animator) return;
        
        auto it = m_states.find(stateName);
        if (it == m_states.end())
        {
            // 스크립트에서 직접 제어하는 경우 LogicFSM 상태와 AnimFSM 상태가 다를 수 있음
            // 로그를 경고 수준으로 낮춤
            // LOG_PRINT("[AnimFSM] State not found: {}", stateName);
            return;
        }
        
        const AnimState& state = it->second;
        m_currentState = stateName;
        
        if (state.useSplitAnimation)
        {
            PlaySplitAnimation(state);
        }
        else
        {
            PlayDefaultAnimation(state);
        }
    }

    void AnimFSM::PlayDefaultAnimation(const AnimState& state)
    {
        m_animator->PlayCrossFade(
            state.animationName,
            state.crossFadeDuration,
            state.loop,
            state.layerIndex,
            state.speed
        );
    }

    void AnimFSM::PlaySplitAnimation(const AnimState& state)
    {
        // 하체 (Base Layer) - 항상 재생
        m_animator->SetLayerWeight(m_baseLayerIndex, 1.0f);
        m_animator->PlayCrossFade(
            state.lowerAnimation,
            state.crossFadeDuration,
            state.lowerLoop,
            m_baseLayerIndex,
            1.0f
        );
        
        // 상체 (Upper Body Layer) - 웨이트에 따라 활성화
        if (state.upperBodyWeight > 0.0f && !state.upperAnimation.empty())
        {
            m_animator->SetLayerWeight(m_upperBodyLayerIndex, state.upperBodyWeight);
            m_animator->Play(state.upperAnimation, state.upperLoop, m_upperBodyLayerIndex, 1.0f);
        }
        else
        {
            // 상체 레이어 비활성화 (하체 애니메이션이 전체에 적용)
            m_animator->SetLayerWeight(m_upperBodyLayerIndex, 0.0f);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::OnGui()
    {
        ImGui::Indent();  // "AnimFSM Component" CollapsingHeader 들여쓰기
        
        if (ImGui::CollapsingHeader("AnimFSM Component", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            
            // ─────────────────────────────────────────────
            // 현재 상태
            // ─────────────────────────────────────────────
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Current State: %s", 
                m_currentState.empty() ? "(none)" : m_currentState.c_str());
            
            if (m_animator)
            {
                float normalizedTime = GetCurrentAnimationNormalizedTime();
                ImGui::Text("Animation Time: %.2f", normalizedTime);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "SkeletalAnimator: NOT FOUND");
            }

            ImGui::Separator();

            // ─────────────────────────────────────────────
            // 레이어 설정
            // ─────────────────────────────────────────────
            ImGui::Text("Layer Settings:");
            ImGui::DragInt("Base Layer", &m_baseLayerIndex, 1, 0, 10);
            ImGui::DragInt("Upper Body Layer", &m_upperBodyLayerIndex, 1, 0, 10);

            ImGui::Separator();

            // ─────────────────────────────────────────────
            // Procedural 조준 설정
            // ─────────────────────────────────────────────
            ImGui::Text("Procedural Aim:");
            ImGui::Checkbox("Enable", &m_enableProceduralAim);
            if (m_enableProceduralAim)
            {
                ImGui::DragFloat("Yaw", &m_upperBodyYaw, 1.0f, -m_maxYaw, m_maxYaw, "%.1f deg");
                ImGui::DragFloat("Pitch", &m_upperBodyPitch, 1.0f, -m_maxPitch, m_maxPitch, "%.1f deg");
                ImGui::DragFloat("Max Yaw", &m_maxYaw, 1.0f, 0.0f, 180.0f, "%.1f deg");
                ImGui::DragFloat("Max Pitch", &m_maxPitch, 1.0f, 0.0f, 90.0f, "%.1f deg");
                ImGui::DragFloat("Lerp Speed", &m_aimLerpSpeed, 0.5f, 1.0f, 50.0f);
                
                char boneBuf[64];
                strcpy_s(boneBuf, m_spineBoneName.c_str());
                if (ImGui::InputText("Spine Bone", boneBuf, 64))
                {
                    m_spineBoneName = boneBuf;
                }
            }

            ImGui::Separator();

            // ─────────────────────────────────────────────
            // 등록된 애니메이션 상태 표시
            // ─────────────────────────────────────────────
            ImGui::Text("Registered States: (%zu)", m_states.size());
            
            if (m_states.empty())
            {
                ImGui::TextDisabled("  (No states registered)");
            }
            else
            {
                for (const auto& [stateName, state] : m_states)
                {
                    bool isCurrent = (stateName == m_currentState);
                    
                    if (isCurrent)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                    }
                    
                    if (state.useSplitAnimation)
                    {
                        // Split 타입
                        ImGui::BulletText("[Split] %s", stateName.c_str());
                        ImGui::Text("    Lower: %s", state.lowerAnimation.c_str());
                        if (!state.upperAnimation.empty())
                        {
                            ImGui::Text("    Upper: %s (weight=%.1f)", 
                                state.upperAnimation.c_str(), state.upperBodyWeight);
                        }
                        else
                        {
                            ImGui::TextDisabled("    Upper: (disabled)");
                        }
                    }
                    else
                    {
                        // Default 타입
                        ImGui::BulletText("[Default] %s -> %s", 
                            stateName.c_str(), state.animationName.c_str());
                    }
                    
                    if (isCurrent)
                    {
                        ImGui::PopStyleColor();
                    }
                }
            }
            
            ImGui::Unindent();
        }
        
        ImGui::Unindent();  // "AnimFSM Component" CollapsingHeader 들여쓰기 종료
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::Save(json& j) const
    {
        Object::Save(j);
        
        // 레이어 설정
        j["BaseLayerIndex"] = m_baseLayerIndex;
        j["UpperBodyLayerIndex"] = m_upperBodyLayerIndex;
        
        // Procedural Aim 설정
        j["EnableProceduralAim"] = m_enableProceduralAim;
        j["MaxYaw"] = m_maxYaw;
        j["MaxPitch"] = m_maxPitch;
        j["AimLerpSpeed"] = m_aimLerpSpeed;
        j["SpineBoneName"] = m_spineBoneName;
        
        // 애니메이션 상태 저장 (통합)
        std::vector<json> statesJson;
        for (const auto& [stateName, state] : m_states)
        {
            json stateNode;
            stateNode["StateName"] = state.stateName;
            stateNode["CrossFadeDuration"] = state.crossFadeDuration;
            stateNode["UseSplitAnimation"] = state.useSplitAnimation;
            
            if (state.useSplitAnimation)
            {
                // Split 타입
                stateNode["LowerAnimation"] = state.lowerAnimation;
                stateNode["LowerLoop"] = state.lowerLoop;
                stateNode["UpperAnimation"] = state.upperAnimation;
                stateNode["UpperLoop"] = state.upperLoop;
                stateNode["UpperBodyWeight"] = state.upperBodyWeight;
            }
            else
            {
                // Default 타입
                stateNode["AnimationName"] = state.animationName;
                stateNode["Loop"] = state.loop;
                stateNode["LayerIndex"] = state.layerIndex;
                stateNode["Speed"] = state.speed;
            }
            
            statesJson.push_back(stateNode);
        }
        j["AnimStates"] = statesJson;
    }

    void AnimFSM::Load(const json& j)
    {
        Object::Load(j);
        
        // 레이어 설정
        JsonGet(j, "BaseLayerIndex", m_baseLayerIndex);
        JsonGet(j, "UpperBodyLayerIndex", m_upperBodyLayerIndex);
        
        // Procedural Aim 설정
        JsonGet(j, "EnableProceduralAim", m_enableProceduralAim);
        JsonGet(j, "MaxYaw", m_maxYaw);
        JsonGet(j, "MaxPitch", m_maxPitch);
        JsonGet(j, "AimLerpSpeed", m_aimLerpSpeed);
        JsonGet(j, "SpineBoneName", m_spineBoneName);
        
        // 애니메이션 상태 로드 (통합)
        m_states.clear();
        JsonArrayForEach(j, "AnimStates", [&](const json& stateNode)
        {
            AnimState state;
            state.stateName = stateNode.value("StateName", "");
            state.crossFadeDuration = stateNode.value("CrossFadeDuration", 0.1f);
            state.useSplitAnimation = stateNode.value("UseSplitAnimation", false);
            
            if (state.useSplitAnimation)
            {
                // Split 타입
                state.lowerAnimation = stateNode.value("LowerAnimation", "");
                state.lowerLoop = stateNode.value("LowerLoop", true);
                state.upperAnimation = stateNode.value("UpperAnimation", "");
                state.upperLoop = stateNode.value("UpperLoop", true);
                state.upperBodyWeight = stateNode.value("UpperBodyWeight", 0.0f);
            }
            else
            {
                // Default 타입
                state.animationName = stateNode.value("AnimationName", "");
                state.loop = stateNode.value("Loop", true);
                state.layerIndex = stateNode.value("LayerIndex", 0);
                state.speed = stateNode.value("Speed", 1.0f);
            }
            
            if (!state.stateName.empty())
            {
                m_states[state.stateName] = state;
            }
        });
        
        //// 하위 호환성: 기존 AnimationMappings 로드
        //JsonArrayForEach(j, "AnimationMappings", [&](const json& mappingNode)
        //{
        //    AnimState state;
        //    state.stateName = mappingNode.value("StateName", "");
        //    state.useSplitAnimation = false;
        //    state.animationName = mappingNode.value("AnimationName", "");
        //    state.crossFadeDuration = mappingNode.value("CrossFadeDuration", 0.2f);
        //    state.loop = mappingNode.value("Loop", true);
        //    state.layerIndex = mappingNode.value("LayerIndex", 0);
        //    state.speed = mappingNode.value("Speed", 1.0f);
        //    
        //    if (!state.stateName.empty())
        //    {
        //        m_states[state.stateName] = state;
        //    }
        //});
    }
}
