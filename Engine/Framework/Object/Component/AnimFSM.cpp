#include "EnginePCH.h"
#include "Framework/Object/Component/AnimFSM.h"
#include "Framework/Object/Component/LogicFSM.h"
#include "Framework/Object/Component/Animator/SkeletalAnimator.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Engine/Core/System/MyTime.h"
#include <algorithm>
#include <cmath>

namespace engine
{
    void AnimFSM::Initialize()
    {
        // 초기화
    }

    void AnimFSM::Awake()
    {
        // SkeletalAnimator 찾기 (항상 같은 GameObject에서)
        m_animator = GetGameObject()->GetComponent<SkeletalAnimator>();
        
        // LogicFSM 찾기
        // m_logicFSMObjectName이 비어있으면 같은 GameObject에서 찾음
        // 값이 있으면 해당 이름의 GameObject에서 찾음
        if (m_logicFSMObjectName.empty())
        {
            m_logicFSM = GetGameObject()->GetComponent<LogicFSM>();
        }
        else
        {
            // 다른 오브젝트에서 LogicFSM 찾기
            auto* scene = SceneManager::Get().GetScene();
            if (scene)
            {
                if (auto* targetGO = scene->FindGameObject(m_logicFSMObjectName))
                {
                    m_logicFSM = targetGO->GetComponent<LogicFSM>();
                }
            }
        }
        
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
        // 상체 레이어 weight 보간 (레이어 간 전환 시 부드럽게)
        if (m_animator)
        {
            float diff = m_upperBodyWeightTarget - m_currentUpperBodyWeight;
            if (std::abs(diff) < 0.001f)
            {
                m_currentUpperBodyWeight = m_upperBodyWeightTarget;
            }
            else
            {
                float step = (Time::DeltaTime() / std::max(0.001f, m_upperBodyWeightRampDuration));
                if (step >= std::abs(diff))
                    m_currentUpperBodyWeight = m_upperBodyWeightTarget;
                else
                    m_currentUpperBodyWeight += (diff > 0.0f ? step : -step);
            }
            m_animator->SetLayerWeight(m_upperBodyLayerIndex, m_currentUpperBodyWeight);

            // 상체가 켜져 있어야 하는데 레이어가 재생 중이 아니면 다시 재생 (꾹 눌러도 Fire 계속 재생되도록)
            if (m_upperBodyWeightTarget > 0.001f && !m_currentState.empty())
            {
                auto it = m_states.find(m_currentState);
                if (it != m_states.end() && it->second.useSplitAnimation && !it->second.upperAnimation.empty())
                {
                    const std::string& expected = it->second.upperAnimation;
                    if (m_animator->GetCurrentAnimationName(m_upperBodyLayerIndex) != expected || !m_animator->IsPlaying(m_upperBodyLayerIndex))
                        m_animator->Play(expected, it->second.upperLoop, m_upperBodyLayerIndex, 1.0f);
                }
            }
        }
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
        if (m_scriptControlled) return;
        if (m_states.find(newState) != m_states.end())
            PlayStateAnimation(newState);
    }

    void AnimFSM::OnLogicStateEntered(const std::string& state)
    {
        if (m_scriptControlled) return;
        if (m_states.find(state) != m_states.end())
            PlayStateAnimation(state);
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
        // Identity는 weight가 0으로 내려간 뒤 UpdateProceduralAim에서만 설정 (걸으며 쏘다가 손 떼면 휙 도는 현상 방지)
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

    void AnimFSM::OffsetCurrentYaw(float offsetDegrees)
    {
        // 하체 회전 보정: 이 프레임에 적용할 오프셋 누적
        // UpdateProceduralAim()에서 m_currentYaw = m_upperBodyYaw + m_yawOffset 후 초기화됨
        m_yawOffset += offsetDegrees;
    }

    void AnimFSM::UpdateProceduralAim()
    {
        // weight가 거의 0이면 프로시저럴 끔 (걸으며 쏘다가 손 뗀 뒤 보간 끝났을 때만 Identity)
        if (m_currentUpperBodyWeight < 0.001f)
        {
            if (m_animator && !m_enableProceduralAim)
                m_animator->SetProceduralRotation(m_spineBoneName, Quaternion::Identity);
            return;
        }
        // 활성화 중일 때만 목표 yaw/pitch 갱신 (꺼진 뒤 weight 보간 중에는 마지막 값 유지)
        if (m_enableProceduralAim)
        {
            m_currentYaw = m_upperBodyYaw + m_yawOffset;
            m_yawOffset = 0.0f;
            m_currentPitch = m_upperBodyPitch;
        }
        ApplyProceduralRotation();
    }

    void AnimFSM::ApplyProceduralRotation()
    {
        if (!m_animator) return;
        
        float yawRad = ToRadian(m_currentYaw);
        float pitchRad = ToRadian(m_currentPitch);
        
        Quaternion yawRot = Quaternion::CreateFromAxisAngle(Vector3::UnitY, yawRad);
        Quaternion pitchRot = Quaternion::CreateFromAxisAngle(Vector3::UnitX, pitchRad);
        Quaternion fullRot = yawRot * pitchRot;
        
        // 상체 weight에 비례해 프로시저럴 회전 적용 (weight 0→1 보간과 동기화)
        float t = std::clamp(m_currentUpperBodyWeight, 0.0f, 1.0f);
        Quaternion finalRot = Quaternion::Slerp(Quaternion::Identity, fullRot, t);
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
        // 단일 레이어 재생 시 상체 레이어 비활성화 (이전 Split 상태에서 전환 시 상체 잔상 제거)
        m_upperBodyWeightTarget = 0.0f;
        m_currentUpperBodyWeight = 0.0f;
        m_animator->SetLayerWeight(m_upperBodyLayerIndex, 0.0f);
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
        
        // 상체 (Upper Body Layer) - 목표 weight만 설정, 실제 weight는 UpdateFSM에서 보간. 프로시저럴은 ApplyProceduralRotation에서 weight에 비례 적용
        if (state.upperBodyWeight > 0.0f && !state.upperAnimation.empty())
        {
            m_upperBodyWeightTarget = state.upperBodyWeight;
            m_animator->Play(state.upperAnimation, state.upperLoop, m_upperBodyLayerIndex, 1.0f);
        }
        else
        {
            m_upperBodyWeightTarget = 0.0f;
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
            // 외부 LogicFSM 참조 설정
            // ─────────────────────────────────────────────
            ImGui::Text("LogicFSM Reference:");
            ImGui::InputText("LogicFSM Object", &m_logicFSMObjectName);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Leave empty to use LogicFSM on same GameObject.\nSet object name to reference LogicFSM on another GameObject.");
            }
            ImGui::Text("LogicFSM: %s", m_logicFSM ? "[OK]" : "[NOT FOUND]");
            
            ImGui::Separator();

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
        
        // 외부 LogicFSM 참조
        j["LogicFSMObjectName"] = m_logicFSMObjectName;
        
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
        
        // 외부 LogicFSM 참조
        JsonGet(j, "LogicFSMObjectName", m_logicFSMObjectName);
        
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
