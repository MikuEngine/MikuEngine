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
        // 상태 변경 시 애니메이션 재생
        PlayStateAnimation(newState);
    }

    void AnimFSM::OnLogicStateEntered(const std::string& state)
    {
        // 상태 진입 시 애니메이션 재생
        PlayStateAnimation(state);
    }

    // ═══════════════════════════════════════════════════════════════
    // 애니메이션 매핑 설정
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::AddAnimationMapping(const AnimationMapping& mapping)
    {
        m_animations[mapping.stateName] = mapping;
    }

    void AnimFSM::RemoveAnimationMapping(const std::string& stateName)
    {
        m_animations.erase(stateName);
    }

    void AnimFSM::ClearMappings()
    {
        m_animations.clear();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상하체 분리 설정
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::SetUpperBodyLayer(bool enabled, int layerIndex)
    {
        m_useUpperBodyLayer = enabled;
        m_upperBodyLayerIndex = layerIndex;
    }

    void AnimFSM::SetUpperBodyWeight(float weight)
    {
        m_upperBodyWeight = weight;
        if (m_animator && m_useUpperBodyLayer)
        {
            m_animator->SetLayerWeight(m_upperBodyLayerIndex, weight);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상하체 분리 재생
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::PlaySplitAnimation(
        const std::string& lowerAnim, bool lowerLoop,
        const std::string& upperAnim, bool upperLoop,
        float crossFade)
    {
        if (!m_animator) return;
        
        // 하체 (베이스 레이어)
        m_animator->PlayCrossFade(lowerAnim, crossFade, lowerLoop, m_baseLayerIndex, 1.0f);
        
        // 상체 (Upper Body 레이어)
        m_animator->SetLayerWeight(m_upperBodyLayerIndex, 1.0f);
        m_animator->Play(upperAnim, upperLoop, m_upperBodyLayerIndex, 1.0f);
        
        m_useUpperBodyLayer = true;
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
    // 애니메이션 정보 조회
    // ═══════════════════════════════════════════════════════════════
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
    // 내부 로직
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::PlayStateAnimation(const std::string& stateName)
    {
        if (!m_animator) return;
        
        auto it = m_animations.find(stateName);
        if (it != m_animations.end())
        {
            const auto& mapping = it->second;
            m_currentState = stateName;
            
            m_animator->PlayCrossFade(
                mapping.animationName,
                mapping.crossFadeDuration,
                mapping.loop,
                mapping.layerIndex,
                mapping.speed
            );
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void AnimFSM::OnGui()
    {
        ImGui::Text("AnimFSM Component");
        ImGui::Text("Current State: %s", m_currentState.c_str());
        
        if (m_animator)
        {
            float normalizedTime = GetCurrentAnimationNormalizedTime();
            ImGui::Text("Animation Time: %.2f", normalizedTime);
        }

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

        // Procedural 조준 설정
        if (ImGui::CollapsingHeader("Procedural Aim"))
        {
            ImGui::Checkbox("Enable Procedural Aim", &m_enableProceduralAim);
            ImGui::DragFloat("Aim Yaw", &m_upperBodyYaw, 1.0f, -m_maxYaw, m_maxYaw, "%.1f deg");
            ImGui::DragFloat("Aim Pitch", &m_upperBodyPitch, 1.0f, -m_maxPitch, m_maxPitch, "%.1f deg");
            ImGui::DragFloat("Max Yaw", &m_maxYaw, 1.0f, 0.0f, 180.0f, "%.1f deg");
            ImGui::DragFloat("Max Pitch", &m_maxPitch, 1.0f, 0.0f, 90.0f, "%.1f deg");
            ImGui::DragFloat("Aim Lerp Speed", &m_aimLerpSpeed, 0.5f, 1.0f, 30.0f);
            
            char boneBuf[64];
            strcpy_s(boneBuf, m_spineBoneName.c_str());
            if (ImGui::InputText("Spine Bone", boneBuf, 64))
            {
                m_spineBoneName = boneBuf;
            }
        }

        // 애니메이션 매핑 표시
        if (ImGui::CollapsingHeader("Animation Mappings"))
        {
            for (const auto& [stateName, mapping] : m_animations)
            {
                ImGui::Text("%s -> %s", stateName.c_str(), mapping.animationName.c_str());
            }
        }
    }

    void AnimFSM::Save(json& j) const
    {
        Object::Save(j);
        
        j["UseUpperBodyLayer"] = m_useUpperBodyLayer;
        j["BaseLayerIndex"] = m_baseLayerIndex;
        j["UpperBodyLayerIndex"] = m_upperBodyLayerIndex;
        j["UpperBodyWeight"] = m_upperBodyWeight;
        
        j["EnableProceduralAim"] = m_enableProceduralAim;
        j["MaxYaw"] = m_maxYaw;
        j["MaxPitch"] = m_maxPitch;
        j["AimLerpSpeed"] = m_aimLerpSpeed;
        j["SpineBoneName"] = m_spineBoneName;
        
        // 애니메이션 매핑 저장
        std::vector<json> mappingsJson;
        for (const auto& [stateName, mapping] : m_animations)
        {
            json mappingNode;
            mappingNode["StateName"] = mapping.stateName;
            mappingNode["AnimationName"] = mapping.animationName;
            mappingNode["CrossFadeDuration"] = mapping.crossFadeDuration;
            mappingNode["Loop"] = mapping.loop;
            mappingNode["LayerIndex"] = mapping.layerIndex;
            mappingNode["Speed"] = mapping.speed;
            mappingsJson.push_back(mappingNode);
        }
        j["AnimationMappings"] = mappingsJson;
    }

    void AnimFSM::Load(const json& j)
    {
        Object::Load(j);
        
        JsonGet(j, "UseUpperBodyLayer", m_useUpperBodyLayer);
        JsonGet(j, "BaseLayerIndex", m_baseLayerIndex);
        JsonGet(j, "UpperBodyLayerIndex", m_upperBodyLayerIndex);
        JsonGet(j, "UpperBodyWeight", m_upperBodyWeight);
        
        JsonGet(j, "EnableProceduralAim", m_enableProceduralAim);
        JsonGet(j, "MaxYaw", m_maxYaw);
        JsonGet(j, "MaxPitch", m_maxPitch);
        JsonGet(j, "AimLerpSpeed", m_aimLerpSpeed);
        JsonGet(j, "SpineBoneName", m_spineBoneName);
        
        // 애니메이션 매핑 로드
        m_animations.clear();
        JsonArrayForEach(j, "AnimationMappings", [&](const json& mappingNode)
        {
            AnimationMapping mapping;
            mapping.stateName = mappingNode.value("StateName", "");
            mapping.animationName = mappingNode.value("AnimationName", "");
            mapping.crossFadeDuration = mappingNode.value("CrossFadeDuration", 0.2f);
            mapping.loop = mappingNode.value("Loop", true);
            mapping.layerIndex = mappingNode.value("LayerIndex", 0);
            mapping.speed = mappingNode.value("Speed", 1.0f);
            
            m_animations[mapping.stateName] = mapping;
        });
    }
}
