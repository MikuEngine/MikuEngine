#pragma once

#include "Framework/Object/Component/Component.h"
#include <unordered_map>
#include <string>

namespace engine
{
    class SkeletalAnimator;
    class LogicFSM;

    // ═══════════════════════════════════════════════════════════════
    // AnimState - 통합 애니메이션 상태
    // 
    // useSplitAnimation = false: 단일 레이어 (Default)
    // useSplitAnimation = true:  상/하체 분리 (Split)
    // ═══════════════════════════════════════════════════════════════
    struct AnimState
    {
        std::string stateName;              // 상태 이름
        float crossFadeDuration = 0.1f;     // 크로스페이드 시간
        bool useSplitAnimation = false;     // true면 상/하체 분리
        
        // ─────────────────────────────────────────────
        // 단일 레이어용 (useSplitAnimation = false)
        // ─────────────────────────────────────────────
        std::string animationName;          // 재생할 애니메이션 이름
        bool loop = true;                   // 루프 여부
        int layerIndex = 0;                 // 레이어 인덱스
        float speed = 1.0f;                 // 재생 속도
        
        // ─────────────────────────────────────────────
        // 상/하체 분리용 (useSplitAnimation = true)
        // ─────────────────────────────────────────────
        std::string lowerAnimation;         // 하체 애니메이션 (Base Layer)
        bool lowerLoop = true;
        std::string upperAnimation;         // 상체 애니메이션 (Upper Layer, 비어있으면 비활성화)
        bool upperLoop = true;
        float upperBodyWeight = 0.0f;       // 상체 레이어 웨이트 (0이면 비활성화)
    };

    // ═══════════════════════════════════════════════════════════════
    // AnimFSM - 애니메이션 상태 머신 컴포넌트
    // 
    // 기능:
    //   - LogicFSM 상태 변화 감지 (자동 연동)
    //   - 스크립트에서 직접 상태 설정 (수동 제어)
    //   - 단일 레이어 / 상하체 분리 애니메이션 지원
    //   - Procedural 상체 회전 지원
    // ═══════════════════════════════════════════════════════════════
    class AnimFSM :
        public Component
    {
        REGISTER_COMPONENT(AnimFSM, Component)

    private:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        SkeletalAnimator* m_animator = nullptr;
        LogicFSM* m_logicFSM = nullptr;
        
        // ─────────────────────────────────────────────
        // 애니메이션 상태 맵 (통합)
        // ─────────────────────────────────────────────
        std::unordered_map<std::string, AnimState> m_states;
        std::string m_currentState;
        
        // ─────────────────────────────────────────────
        // 레이어 설정
        // ─────────────────────────────────────────────
        int m_baseLayerIndex = 0;
        int m_upperBodyLayerIndex = 1;
        
        // ─────────────────────────────────────────────
        // Procedural 상체 회전
        // ─────────────────────────────────────────────
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
        void UpdateFSM();

        // ─────────────────────────────────────────────
        // LogicFSM 연동
        // ─────────────────────────────────────────────
        void SetLogicFSM(LogicFSM* logicFSM);
        void OnLogicStateChanged(const std::string& oldState, const std::string& newState);
        void OnLogicStateEntered(const std::string& state);

        // ─────────────────────────────────────────────
        // 애니메이션 상태 등록 (통합 API)
        // ─────────────────────────────────────────────
        void AddState(const AnimState& state);
        void RemoveState(const std::string& stateName);
        void ClearStates();
        
        // 단일 레이어 상태 추가 (편의 함수)
        void AddDefaultState(
            const std::string& stateName,
            const std::string& animationName,
            bool loop = true,
            float crossFade = 0.1f,
            int layerIndex = 0,
            float speed = 1.0f
        );
        
        // 상/하체 분리 상태 추가 (편의 함수)
        void AddSplitState(
            const std::string& stateName,
            const std::string& lowerAnim, bool lowerLoop,
            const std::string& upperAnim, bool upperLoop,
            float upperWeight = 0.0f,
            float crossFade = 0.1f
        );

        // ─────────────────────────────────────────────
        // 레이어 설정
        // ─────────────────────────────────────────────
        void SetLayerIndices(int baseLayer, int upperBodyLayer);
        int GetBaseLayerIndex() const { return m_baseLayerIndex; }
        int GetUpperBodyLayerIndex() const { return m_upperBodyLayerIndex; }

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
        // 상태 조회 및 제어
        // ─────────────────────────────────────────────
        std::string GetCurrentState() const { return m_currentState; }
        void SetAnimState(const std::string& stateName);
        
        // 상체 애니메이션만 다시 재생 (공격 등 액션 트리거용)
        void PlayUpperBodyAnimation(const std::string& animName, bool loop = false);
        
        float GetCurrentAnimationNormalizedTime() const;
        bool IsAnimationFinished() const;

        // ─────────────────────────────────────────────
        // 애니메이션 종료 알림 (LogicFSM 연동용)
        // ─────────────────────────────────────────────
        void NotifyAnimationFinished(const std::string& animationName);

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    private:
        void PlayStateAnimation(const std::string& stateName);
        void PlayDefaultAnimation(const AnimState& state);
        void PlaySplitAnimation(const AnimState& state);
        void UpdateProceduralAim();
        void ApplyProceduralRotation();
    };
}
