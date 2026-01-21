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
    // CharacterAnimationFSM - 캐릭터 애니메이션 FSM 베이스 클래스
    // 
    // 자식 클래스에서 구현해야 할 것:
    //   - SetupAnimationMappings(): 상태별 애니메이션 매핑 설정
    //   - Update(): 애니메이션 종료 체크 및 NotifyAnimationFinished 호출
    //   - OnStateEnter(): 상태 진입 시 애니메이션 재생 (필요시 오버라이드)
    // ═══════════════════════════════════════════════════════════════
    class CharacterAnimationFSM :
        public engine::Script<CharacterAnimationFSM>,
        public ILogicFSMListener
    {
        REGISTER_COMPONENT(CharacterAnimationFSM)

    protected:
        // 컴포넌트 캐싱
        engine::SkeletalAnimator* m_animator = nullptr;
        CharacterLogicFSM* m_logicFSM = nullptr;
        
        // 상태별 애니메이션 매핑
        std::unordered_map<CharacterState, AnimationTransition> m_stateAnimations;
        
        // 현재 재생 중인 상태
        CharacterState m_currentAnimState = CharacterState::Idle;
        
        // 상체/하체 분리 설정
        bool m_useUpperBodyLayer = false;
        int m_baseLayerIndex = 0;
        int m_upperBodyLayerIndex = 1;
        float m_upperBodyWeight = 1.0f;
        
        // 기본 크로스페이드 시간
        float m_defaultCrossFade = 0.2f;
        
        // 조건부 전이용 데이터
        float m_moveBlendThreshold = 0.1f;  // Walk/Run 블렌딩 임계값

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        
        // ILogicFSMListener 구현 (자식에서 오버라이드)
        void OnStateEnter(const StateContext& context) override;
        void OnStateExit(const StateContext& context) override;
        void OnStateUpdate(const StateContext& context) override;
        
        // 애니메이션 매핑 설정
        void SetStateAnimation(CharacterState state, const AnimationTransition& transition);
        void SetStateAnimation(CharacterState state, const std::string& animName, 
            float crossFade = 0.2f, bool loop = true, float speed = 1.0f);
        
        // 상체 분리 설정
        void SetUpperBodyLayer(bool enabled, int layerIndex = 1);
        void SetUpperBodyWeight(float weight);
        
        // 직접 애니메이션 재생 (자식 클래스에서 사용)
        void PlayAnimation(const std::string& animName, float crossFade = 0.2f, 
            bool loop = true, int layerIndex = 0, float speed = 1.0f);
        
        // 로직 FSM에 애니메이션 종료 알림 (자식 클래스에서 호출)
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
        
        // 조건부 전이 처리 (자식에서 오버라이드 가능)
        virtual void HandleConditionalTransition(const StateContext& context);
        virtual void UpdateMoveBlending(const StateContext& context);
    };
}
