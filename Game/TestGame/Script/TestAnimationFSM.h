#pragma once

#include "CharacterAnimationFSM.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // TestAnimationFSM - 스켈레탈 애니메이션 테스트용 애니메이션 FSM
    // 
    // 기능:
    // - Test1: 기본 재생 후 Idle 복귀
    // - Test2: 70% 진행 시 조기 종료 후 Idle 복귀
    // - Test3: 상체(펀치) + 하체(걷기) 분리 재생 후 Idle 복귀
    // - Test4: 애니메이션 종료 후 1초 대기 후 Idle 복귀
    // ═══════════════════════════════════════════════════════════════

    class TestAnimationFSM :
        public CharacterAnimationFSM
    {
        REGISTER_COMPONENT(TestAnimationFSM, Script)

    private:
        // Test4용 대기 타이머
        bool m_isWaitingAfterTest4 = false;
        float m_test4WaitTimer = 0.0f;
        float m_test4WaitDuration = 1.0f;
        
        // Test3 재진입 방지용 (애니메이션 시작 확인)
        bool m_test3Started = false;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        // ILogicFSMListener 오버라이드
        void OnStateEnter(const StateContext& context) override;
        void OnStateExit(const StateContext& context) override;

    private:
        void SetupAnimationMappings();
        void CheckTest2EarlyExit();
        void CheckTest1Finished();
        void CheckTest3Finished();
        void CheckTest4Finished();
        void UpdateTest4Wait();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
