#pragma once

#include <Framework/Object/Component/Script.h>
#include "CharacterLogicFSM.h"

namespace engine
{
    class SkeletalAnimator;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // AnimationTransition - 애니메이션 전환 설정
    // ═══════════════════════════════════════════════════════════════
    struct AnimationTransition
    {
        std::string animationName;          // 재생할 애니메이션 이름
        float crossFadeDuration = 0.2f;     // 크로스페이드 시간
        bool loop = true;                   // 루프 여부
        int layerIndex = 0;                 // 레이어 인덱스
        float speed = 1.0f;                 // 재생 속도
    };

    // ═══════════════════════════════════════════════════════════════
    // LayerAnimationState - 레이어별 애니메이션 상태 추적
    // ═══════════════════════════════════════════════════════════════
    struct LayerAnimationState
    {
        std::string animationName;
        bool isPlaying = false;
        bool isLooping = false;
        float exitNormalizedTime = 1.0f;    // 종료 판정 시점 (0.0 ~ 1.0)
        bool waitForFinish = true;          // 종료 대기 여부
        bool hasStarted = false;            // 애니메이션 시작 확인 (재진입 방지)
        
        void Reset()
        {
            animationName.clear();
            isPlaying = false;
            isLooping = false;
            exitNormalizedTime = 1.0f;
            waitForFinish = true;
            hasStarted = false;
        }
    };

    // ═══════════════════════════════════════════════════════════════
    // CharacterAnimationFSM - 캐릭터 애니메이션 FSM 베이스 클래스
    // 
    // 기능:
    //   - 상태별 애니메이션 매핑
    //   - 상하체 분리 애니메이션 지원
    //   - 상체 Procedural 회전 (조준 등)
    //   - 레이어별 애니메이션 종료 콜백
    // ═══════════════════════════════════════════════════════════════
    class CharacterAnimationFSM :
        public engine::Script<CharacterAnimationFSM>,
        public ILogicFSMListener
    {
        REGISTER_COMPONENT(CharacterAnimationFSM)

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 캐싱
        // ─────────────────────────────────────────────
        engine::SkeletalAnimator* m_animator = nullptr;
        CharacterLogicFSM* m_logicFSM = nullptr;
        
        // ─────────────────────────────────────────────
        // 상태별 애니메이션 매핑
        // ─────────────────────────────────────────────
        std::unordered_map<CharacterState, AnimationTransition> m_stateAnimations;
        CharacterState m_currentAnimState = CharacterState::Idle;
        
        // ─────────────────────────────────────────────
        // 레이어 설정
        // ─────────────────────────────────────────────
        bool m_useUpperBodyLayer = false;
        int m_baseLayerIndex = 0;
        int m_upperBodyLayerIndex = 1;
        float m_upperBodyWeight = 1.0f;
        float m_defaultCrossFade = 0.2f;
        
        // ─────────────────────────────────────────────
        // 레이어별 애니메이션 상태 (상하체 분리용)
        // ─────────────────────────────────────────────
        LayerAnimationState m_baseLayerState;
        LayerAnimationState m_upperLayerState;
        
        // ─────────────────────────────────────────────
        // 상체 Procedural 회전 (조준)
        // ─────────────────────────────────────────────
        float m_upperBodyYaw = 0.0f;        // Y축 회전 (좌우 조준) - degree
        float m_upperBodyPitch = 0.0f;      // X축 회전 (상하 조준) - degree
        float m_maxYaw = 70.0f;             // 최대 좌우 회전 - degree
        float m_maxPitch = 45.0f;           // 최대 상하 회전 - degree
        float m_aimLerpSpeed = 10.0f;       // 조준 보간 속도
        float m_currentYaw = 0.0f;          // 현재 보간된 Yaw
        float m_currentPitch = 0.0f;        // 현재 보간된 Pitch
        std::string m_spineBoneName = "mixamorig:Spine1";  // 회전 적용할 본
        bool m_enableProceduralAim = false; // Procedural 조준 활성화
        
        // ─────────────────────────────────────────────
        // 조건부 전이용 데이터
        // ─────────────────────────────────────────────
        float m_moveBlendThreshold = 0.1f;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        
        // ─────────────────────────────────────────────
        // ILogicFSMListener 구현
        // ─────────────────────────────────────────────
        void OnStateEnter(const StateContext& context) override;
        void OnStateExit(const StateContext& context) override;
        void OnStateUpdate(const StateContext& context) override;
        
        // ─────────────────────────────────────────────
        // 애니메이션 매핑 설정
        // ─────────────────────────────────────────────
        void SetStateAnimation(CharacterState state, const AnimationTransition& transition);
        void SetStateAnimation(CharacterState state, const std::string& animName, 
            float crossFade = 0.2f, bool loop = true, float speed = 1.0f);
        
        // ─────────────────────────────────────────────
        // 레이어 설정
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
        
        // 레이어별 종료 조건 설정
        void SetBaseLayerExitCondition(float normalizedTime);
        void SetUpperLayerExitCondition(float normalizedTime);
        
        // ─────────────────────────────────────────────
        // 상체 Procedural 회전 (조준)
        // ─────────────────────────────────────────────
        void SetProceduralAimEnabled(bool enabled);
        void SetUpperBodyYaw(float degrees);
        void SetUpperBodyPitch(float degrees);
        void SetUpperBodyAim(float yawDegrees, float pitchDegrees);
        void SetAimLimits(float maxYaw, float maxPitch);
        void SetSpineBoneName(const std::string& boneName);
        
        // ─────────────────────────────────────────────
        // 직접 애니메이션 재생
        // ─────────────────────────────────────────────
        void PlayAnimation(const std::string& animName, float crossFade = 0.2f, 
            bool loop = true, int layerIndex = 0, float speed = 1.0f);
        
        // 로직 FSM에 애니메이션 종료 알림
        void NotifyAnimationFinished(CharacterState state);
        
        // 현재 애니메이션 상태 조회
        CharacterState GetCharacterState() const { return m_currentAnimState; }
        
    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;

    protected:
        virtual void CacheComponents();
        virtual void SetupDefaultMappings();
        
        // 매핑 테이블에서 애니메이션 재생
        void PlayStateAnimation(CharacterState state);
        
        // 조건부 전이 처리
        virtual void HandleConditionalTransition(const StateContext& context);
        virtual void UpdateMoveBlending(const StateContext& context);
        
        // ─────────────────────────────────────────────
        // 레이어별 종료 콜백 (자식에서 오버라이드)
        // ─────────────────────────────────────────────
        virtual void OnBaseLayerFinished();
        virtual void OnUpperLayerFinished();
        
        // 레이어 애니메이션 종료 체크 (Update에서 호출)
        void CheckLayerAnimationFinished();
        
        // Procedural 회전 적용
        void UpdateProceduralAim();
        void ApplyProceduralRotation();
    };
}
