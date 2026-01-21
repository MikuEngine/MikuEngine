#pragma once

#include "CharacterLogicFSM.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // TestLogicFSM - 스켈레탈 애니메이션 테스트용 로직 FSM
    // 
    // 기능: 키보드 입력으로 이동 + 공격
    // ═══════════════════════════════════════════════════════════════

    class TestLogicFSM :
        public CharacterLogicFSM
    {
        REGISTER_COMPONENT(TestLogicFSM)

    private:
        // 테스트용 설정
        float m_rotationSpeed = 180.0f;  // 회전 속도 (도/초)
        bool m_rotateToMoveDirection = true;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    protected:
        void UpdateWalk() override;
        void OnEnterAttack() override;
        void UpdateAttack() override;

    private:
        void HandleRotation();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}
