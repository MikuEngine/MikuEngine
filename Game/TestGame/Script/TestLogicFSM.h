#pragma once

#include "CharacterLogicFSM.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // TestLogicFSM - 스켈레탈 애니메이션 테스트용 로직 FSM
    // 
    // 기능: 숫자키 1~4를 눌러 Test1~Test4 상태로 전환
    //       애니메이션 종료 시 Idle로 복귀
    // ═══════════════════════════════════════════════════════════════
    class TestLogicFSM :
        public CharacterLogicFSM
    {
        REGISTER_COMPONENT(TestLogicFSM, Script)

    public:
        void Awake() override;
        void Start() override;
        
        // 애니메이션 종료 시 Idle로 복귀
        void OnAnimationFinished(CharacterState finishedState) override;

    protected:
        void ProcessInput() override;
        void UpdateCurrentState() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
