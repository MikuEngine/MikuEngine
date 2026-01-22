#pragma once

#include "Framework/Object/Component/Component.h"
#include <unordered_map>
#include <string>

namespace engine
{
    class SkeletalAnimator;
    class LogicFSM;

    // ═══════════════════════════════════════════════════════════════
    // AnimationMapping - 상태별 애니메이션 매핑 (에디터에서 설정)
    // ═══════════════════════════════════════════════════════════════
    struct AnimationMapping
    {
        std::string stateName;          // LogicFSM 상태 이름
        std::string animationName;      // 재생할 애니메이션 이름
        float crossFadeDuration = 0.2f; // 크로스페이드 시간
        bool loop = true;               // 루프 여부
        int layerIndex = 0;             // 레이어 인덱스
        float speed = 1.0f;             // 재생 속도
    };

    // ═══════════════════════════════════════════════════════════════
    // AnimFSM - 애니메이션 상태 머신 컴포넌트
    // 
    // 기능:
    //   - LogicFSM 상태 변화 감지
    //   - 상태별 애니메이션 재생 (에디터에서 설정)
    //   - 상하체 분리 애니메이션 지원
    //   - Procedural 상체 회전 지원
    // ═══════════════════════════════════════════════════════════════
    class AnimFSM :
        public Component
    {
        REGISTER_COMPONENT(AnimFSM, Component)

    private:
        // 컴포넌트 참조
        SkeletalAnimator* m_animator = nullptr;
        LogicFSM* m_logicFSM = nullptr;
        
        // 애니메이션 매핑 (에디터에서 설정)
        std::unordered_map<std::string, AnimationMapping> m_animations;
        
        // 현재 재생 중인 상태
        std::string m_currentState;
        
        // 상하체 분리 설정
        bool m_useUpperBodyLayer = false;
        int m_baseLayerIndex = 0;
        int m_upperBodyLayerIndex = 1;
        float m_upperBodyWeight = 1.0f;
        
        // Procedural 상체 회전
        bool m_enableProceduralAim = false;
        float m_upperBodyYaw = 0.0f;
        float m_upperBodyPitch = 0.0f;
        float m_maxYaw = 70.0f;
        float m_maxPitch = 45.0f;
        float m_aimLerpSpeed = 10.0f;
        float m_currentYaw = 0.0f;
        float m_currentPitch = 0.0f;
        std::string m_spineBoneName = "mixamorig:Spine1";

    public:
        void Initialize() override;
        void Awake() override;
        
        // Update는 가상 함수가 아니므로 개별 함수로 제공
        // BaseControllerScript나 다른 Script에서 호출해야 함
        void UpdateFSM();

        // ─────────────────────────────────────────────
        // LogicFSM 연동
        // ─────────────────────────────────────────────
        void SetLogicFSM(LogicFSM* logicFSM);
        void OnLogicStateChanged(const std::string& oldState, const std::string& newState);
        void OnLogicStateEntered(const std::string& state);

        // ─────────────────────────────────────────────
        // 애니메이션 매핑 설정 (에디터/직렬화용)
        // ─────────────────────────────────────────────
        void AddAnimationMapping(const AnimationMapping& mapping);
        void RemoveAnimationMapping(const std::string& stateName);
        void ClearMappings();

        // ─────────────────────────────────────────────
        // 상하체 분리 설정
        // ─────────────────────────────────────────────
        void SetUpperBodyLayer(bool enabled, int layerIndex = 1);
        void SetUpperBodyWeight(float weight);

        // ─────────────────────────────────────────────
        // 상하체 분리 재생
        // ─────────────────────────────────────────────
        void PlaySplitAnimation(
            const std::string& lowerAnim, bool lowerLoop,
            const std::string& upperAnim, bool upperLoop,
            float crossFade = 0.1f
        );

        // ─────────────────────────────────────────────
        // Procedural 상체 회전 (조준)
        // ─────────────────────────────────────────────
        void SetProceduralAimEnabled(bool enabled);
        void SetUpperBodyYaw(float degrees);
        void SetUpperBodyPitch(float degrees);
        void SetUpperBodyAim(float yawDegrees, float pitchDegrees);
        void SetAimLimits(float maxYaw, float maxPitch);
        void SetSpineBoneName(const std::string& boneName);

        // ─────────────────────────────────────────────
        // 애니메이션 정보 조회
        // ─────────────────────────────────────────────
        std::string GetCurrentState() const { return m_currentState; }
        float GetCurrentAnimationNormalizedTime() const;
        bool IsAnimationFinished() const;

        // ─────────────────────────────────────────────
        // 애니메이션 종료 알림 (LogicFSM에 전달)
        // ─────────────────────────────────────────────
        void NotifyAnimationFinished(const std::string& animationName);

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        //std::string GetType() const override;

    private:
        void PlayStateAnimation(const std::string& stateName);
        void UpdateProceduralAim();
        void ApplyProceduralRotation();
    };
}
