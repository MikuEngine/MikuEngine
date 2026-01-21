#pragma once

#include "CharacterAnimationFSM.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // TestAnimationFSM - 스켈레탈 애니메이션 테스트용 애니메이션 FSM
    // 
    // 기능: LogicFSM 상태에 따른 애니메이션 재생
    // ═══════════════════════════════════════════════════════════════

    class TestAnimationFSM :
        public CharacterAnimationFSM
    {
        REGISTER_COMPONENT(TestAnimationFSM)

    private:
        // 공격 타이머 (애니메이션 종료 감지용)
        float m_attackTimer = 0.0f;
        float m_attackDuration = 0.5f;  // 공격 애니메이션 예상 길이

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        // ILogicFSMListener 오버라이드
        void OnStateEnter(const StateContext& context) override;
        void OnStateUpdate(const StateContext& context) override;
             

    private:
        void SetupAnimationMappings();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}
