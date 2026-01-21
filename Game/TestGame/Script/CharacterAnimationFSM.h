#pragma once

#include <Framework/Object/Component/Script.h>
#include "CharacterLogicFSM.h"

namespace engine
{
    class SkeletalAnimator;
}

namespace game
{
    // 애니메이션 전환 설정
    struct AnimationTransition
    {
        std::string animationName;          // 재생할 애니메이션 이름
        float crossFadeDuration = 0.2f;     // 크로스페이드 시간
        bool loop = true;                   // 루프 여부
        int layerIndex = 0;                 // 레이어 인덱스
        float speed = 1.0f;                 // 재생 속도
    };

    // 상태별 애니메이션 매핑
    struct StateAnimationMapping
    {
        CharacterState state;
        AnimationTransition transition;
    };

    class CharacterAnimationFSM :
        public engine::Script<CharacterAnimationFSM>,
        public ILogicFSMListener
    {
        REGISTER_COMPONENT(CharacterAnimationFSM)

    private:
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
        
        // 콤보 공격 애니메이션 목록
        std::vector<std::string> m_comboAttackAnims;
        
        // 블렌딩 설정
        float m_defaultCrossFade = 0.2f;
        
        // 조건부 전이용 데이터
        float m_moveBlendThreshold = 0.1f;  // Walk/Run 블렌딩 임계값

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        
        // ILogicFSMListener 구현
        void OnStateEnter(const StateContext& context) override;
        void OnStateExit(const StateContext& context) override;
        void OnStateUpdate(const StateContext& context) override;
        
        // 애니메이션 매핑 설정
        void SetStateAnimation(CharacterState state, const AnimationTransition& transition);
        void SetStateAnimation(CharacterState state, const std::string& animName, 
            float crossFade = 0.2f, bool loop = true, float speed = 1.0f);
        
        // 콤보 공격 설정
        void SetComboAttackAnimations(const std::vector<std::string>& anims);
        
        // 상체 분리 설정
        void SetUpperBodyLayer(bool enabled, int layerIndex = 1);
        void SetUpperBodyWeight(float weight);
        
        // 직접 애니메이션 재생 (특수한 경우용)
        void PlayAnimation(const std::string& animName, float crossFade = 0.2f, 
            bool loop = true, int layerIndex = 0, float speed = 1.0f);
        
        // 로직 FSM에 애니메이션 종료 알림
        void NotifyAnimationFinished(CharacterState state);
        
    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;

        const CharacterState& GetCharacterState() const;

    private:
        void CacheComponents();
        void SetupDefaultMappings();
        void PlayStateAnimation(CharacterState state, const StateContext& context);
        std::string GetAnimationForCombo(int comboCount) const;
        
        // 조건부 전이 처리
        void HandleConditionalTransition(const StateContext& context);
        void UpdateMoveBlending(const StateContext& context);
    };
}
