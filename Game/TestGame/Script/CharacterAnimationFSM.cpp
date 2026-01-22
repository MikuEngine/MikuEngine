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
        // 레이어 애니메이션 종료 체크
        CheckLayerAnimationFinished();
        
        // Procedural 조준 업데이트
        UpdateProceduralAim();
    }

    // ═══════════════════════════════════════════════════════════════
    // ILogicFSMListener 구현
    // ═══════════════════════════════════════════════════════════════
    void CharacterAnimationFSM::OnStateEnter(const StateContext& context)
    {
        m_currentAnimState = context.currentState;
        PlayStateAnimation(context.currentState);
    }

    void CharacterAnimationFSM::OnStateExit(const StateContext& context)
    {
        // 레이어 상태 리셋
        m_baseLayerState.Reset();
        m_upperLayerState.Reset();
    }

    void CharacterAnimationFSM::OnStateUpdate(const StateContext& context)
    {
        HandleConditionalTransition(context);
    }

    void CharacterAnimationFSM::HandleConditionalTransition(const StateContext& context)
    {
        UpdateMoveBlending(context);
    }

    void CharacterAnimationFSM::UpdateMoveBlending(const StateContext& context)
    {
        if (context.currentState != CharacterState::Walk)
        {
            return;
        }
        // 속도 기반 블렌딩 (필요시 자식 클래스에서 구현)
    }

    // ═══════════════════════════════════════════════════════════════
    // 애니메이션 매핑 설정
    // ═══════════════════════════════════════════════════════════════
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

    // ═══════════════════════════════════════════════════════════════
    // 레이어 설정
    // ═══════════════════════════════════════════════════════════════
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

    // ═══════════════════════════════════════════════════════════════
    // 상하체 분리 재생
    // ═══════════════════════════════════════════════════════════════
    void CharacterAnimationFSM::PlaySplitAnimation(
        const std::string& lowerAnim, bool lowerLoop,
        const std::string& upperAnim, bool upperLoop,
        float crossFade)
    {
        if (!m_animator) return;
        
        // 하체 (베이스 레이어)
        m_animator->PlayCrossFade(lowerAnim, crossFade, lowerLoop, m_baseLayerIndex, 1.0f);
        m_baseLayerState.animationName = lowerAnim;
        m_baseLayerState.isPlaying = true;
        m_baseLayerState.isLooping = lowerLoop;
        m_baseLayerState.hasStarted = false;
        
        // 상체 (Upper Body 레이어)
        m_animator->SetLayerWeight(m_upperBodyLayerIndex, 1.0f);
        m_animator->Play(upperAnim, upperLoop, m_upperBodyLayerIndex, 1.0f);  // Play로 리셋
        m_upperLayerState.animationName = upperAnim;
        m_upperLayerState.isPlaying = true;
        m_upperLayerState.isLooping = upperLoop;
        m_upperLayerState.hasStarted = false;
        
        m_useUpperBodyLayer = true;
    }

    void CharacterAnimationFSM::SetBaseLayerExitCondition(float normalizedTime)
    {
        m_baseLayerState.exitNormalizedTime = normalizedTime;
    }

    void CharacterAnimationFSM::SetUpperLayerExitCondition(float normalizedTime)
    {
        m_upperLayerState.exitNormalizedTime = normalizedTime;
    }

    // ═══════════════════════════════════════════════════════════════
    // 상체 Procedural 회전 (조준)
    // ═══════════════════════════════════════════════════════════════
    void CharacterAnimationFSM::SetProceduralAimEnabled(bool enabled)
    {
        m_enableProceduralAim = enabled;
        
        // 비활성화 시 회전 리셋
        if (!enabled && m_animator)
        {
            m_animator->SetProceduralRotation(m_spineBoneName, engine::Quaternion::Identity);
        }
    }

    void CharacterAnimationFSM::SetUpperBodyYaw(float degrees)
    {
        m_upperBodyYaw = std::clamp(degrees, -m_maxYaw, m_maxYaw);
    }

    void CharacterAnimationFSM::SetUpperBodyPitch(float degrees)
    {
        m_upperBodyPitch = std::clamp(degrees, -m_maxPitch, m_maxPitch);
    }

    void CharacterAnimationFSM::SetUpperBodyAim(float yawDegrees, float pitchDegrees)
    {
        m_upperBodyYaw = std::clamp(yawDegrees, -m_maxYaw, m_maxYaw);
        m_upperBodyPitch = std::clamp(pitchDegrees, -m_maxPitch, m_maxPitch);
    }

    void CharacterAnimationFSM::SetAimLimits(float maxYaw, float maxPitch)
    {
        m_maxYaw = maxYaw;
        m_maxPitch = maxPitch;
    }

    void CharacterAnimationFSM::SetSpineBoneName(const std::string& boneName)
    {
        m_spineBoneName = boneName;
    }

    void CharacterAnimationFSM::UpdateProceduralAim()
    {
        if (!m_enableProceduralAim) return;
        
        float dt = engine::Time::DeltaTime();
        
        // 부드러운 보간
        m_currentYaw = std::lerp(m_currentYaw, m_upperBodyYaw, m_aimLerpSpeed * dt);
        m_currentPitch = std::lerp(m_currentPitch, m_upperBodyPitch, m_aimLerpSpeed * dt);
        
        ApplyProceduralRotation();
    }

    void CharacterAnimationFSM::ApplyProceduralRotation()
    {
        if (!m_animator) return;
        
        // degree → radian
        float yawRad = engine::ToRadian(m_currentYaw);
        float pitchRad = engine::ToRadian(m_currentPitch);
        
        // Quaternion 생성 (Y축 회전 * X축 회전)
        engine::Quaternion yawRot = engine::Quaternion::CreateFromAxisAngle(engine::Vector3::UnitY, yawRad);
        engine::Quaternion pitchRot = engine::Quaternion::CreateFromAxisAngle(engine::Vector3::UnitX, pitchRad);
        engine::Quaternion finalRot = yawRot * pitchRot;
        
        // SkeletalAnimator에 적용
        m_animator->SetProceduralRotation(m_spineBoneName, finalRot);
    }

    // ═══════════════════════════════════════════════════════════════
    // 애니메이션 재생 및 알림
    // ═══════════════════════════════════════════════════════════════
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
        SetStateAnimation(CharacterState::Idle, "Idle", 0.2f, true);
        SetStateAnimation(CharacterState::Walk, "Walk", 0.2f, true);
        SetStateAnimation(CharacterState::Attack, "Attack", 0.1f, false);
    }

    void CharacterAnimationFSM::PlayStateAnimation(CharacterState state)
    {
        if (!m_animator) return;

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

    // ═══════════════════════════════════════════════════════════════
    // 레이어별 종료 콜백
    // ═══════════════════════════════════════════════════════════════
    void CharacterAnimationFSM::OnBaseLayerFinished()
    {
        // 자식 클래스에서 오버라이드
    }

    void CharacterAnimationFSM::OnUpperLayerFinished()
    {
        // 자식 클래스에서 오버라이드
        // 기본: 상체 레이어 가중치 0으로 리셋
        if (m_animator)
        {
            m_animator->SetLayerWeight(m_upperBodyLayerIndex, 0.0f);
        }
    }

    void CharacterAnimationFSM::CheckLayerAnimationFinished()
    {
        if (!m_animator) return;
        
        // 베이스 레이어 종료 체크
        if (m_baseLayerState.isPlaying && !m_baseLayerState.isLooping)
        {
            float time = m_animator->GetNormalizedTime(m_baseLayerIndex);
            
            // 시작 확인
            if (!m_baseLayerState.hasStarted && time < 0.5f)
            {
                m_baseLayerState.hasStarted = true;
            }
            
            // 종료 확인
            if (m_baseLayerState.hasStarted && time >= m_baseLayerState.exitNormalizedTime)
            {
                m_baseLayerState.isPlaying = false;
                OnBaseLayerFinished();
            }
        }
        
        // 상체 레이어 종료 체크
        if (m_upperLayerState.isPlaying && !m_upperLayerState.isLooping)
        {
            float time = m_animator->GetNormalizedTime(m_upperBodyLayerIndex);
            
            // 시작 확인
            if (!m_upperLayerState.hasStarted && time < 0.5f)
            {
                m_upperLayerState.hasStarted = true;
            }
            
            // 종료 확인
            if (m_upperLayerState.hasStarted && time >= m_upperLayerState.exitNormalizedTime)
            {
                m_upperLayerState.isPlaying = false;
                OnUpperLayerFinished();
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
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
            
            ImGui::Text("Current: Yaw=%.1f, Pitch=%.1f", m_currentYaw, m_currentPitch);
        }
        
        // 레이어 상태
        if (ImGui::CollapsingHeader("Layer States"))
        {
            ImGui::Text("Base Layer: %s %s", 
                m_baseLayerState.animationName.c_str(),
                m_baseLayerState.isPlaying ? "(playing)" : "");
            ImGui::Text("Upper Layer: %s %s",
                m_upperLayerState.animationName.c_str(),
                m_upperLayerState.isPlaying ? "(playing)" : "");
        }
        
        // 상태별 애니메이션 매핑
        if (ImGui::CollapsingHeader("State Animations"))
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
        
        // Procedural Aim
        j["EnableProceduralAim"] = m_enableProceduralAim;
        j["MaxYaw"] = m_maxYaw;
        j["MaxPitch"] = m_maxPitch;
        j["AimLerpSpeed"] = m_aimLerpSpeed;
        j["SpineBoneName"] = m_spineBoneName;
        
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
        
        // Procedural Aim
        engine::JsonGet(j, "EnableProceduralAim", m_enableProceduralAim);
        engine::JsonGet(j, "MaxYaw", m_maxYaw);
        engine::JsonGet(j, "MaxPitch", m_maxPitch);
        engine::JsonGet(j, "AimLerpSpeed", m_aimLerpSpeed);
        engine::JsonGet(j, "SpineBoneName", m_spineBoneName);
        
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
