#include "GamePCH.h"
#include "TestLogicFSM.h"
#include "TestAnimationFSM.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Engine/Core/System/Input.h>

namespace game
{
    void TestLogicFSM::Awake()
    {
        CharacterLogicFSM::Awake();
    }

    void TestLogicFSM::Start()
    {
        CharacterLogicFSM::Start();
    }

    void TestLogicFSM::ProcessInput()
    {
        // 숫자키 1~4 입력으로 Test 상태 전환
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            ChangeState(CharacterState::Test1);
        }
        else if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            ChangeState(CharacterState::Test2);
        }
        else if (engine::Input::IsKeyPressed(engine::Keys::D3))
        {
            ChangeState(CharacterState::Test3);
        }
        else if (engine::Input::IsKeyPressed(engine::Keys::D4))
        {
            ChangeState(CharacterState::Test4);
        }
    }

    void TestLogicFSM::UpdateCurrentState()
    {
        switch (m_currentState)
        {
        case CharacterState::Idle:
            // Idle에서는 Test 입력만 대기 (ProcessInput에서 처리)
            break;
            
        case CharacterState::Test1:
        case CharacterState::Test2:
        case CharacterState::Test3:
        case CharacterState::Test4:
            // Test 상태는 애니메이션 FSM이 종료 알림을 보낼 때까지 대기
            break;
            
        default:
            // 기본 상태(Walk, Attack 등)는 부모 처리
            CharacterLogicFSM::UpdateCurrentState();
            break;
        }
    }

    void TestLogicFSM::OnAnimationFinished(CharacterState finishedState)
    {
        // Test1~Test4 애니메이션 종료 시 Idle로 복귀
        switch (finishedState)
        {
        case CharacterState::Test1:
        case CharacterState::Test2:
        case CharacterState::Test3:
        case CharacterState::Test4:
            if (m_currentState == finishedState)
            {
                ChangeState(CharacterState::Idle);
            }
            break;
            
        default:
            CharacterLogicFSM::OnAnimationFinished(finishedState);
            break;
        }
    }

    void TestLogicFSM::OnGui()
    {
        ImGui::Text("Current State: %s", CharacterStateToString(m_currentState));
        ImGui::Separator();
        ImGui::Text("Press 1~4 for test animations");
    }

    void TestLogicFSM::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TestLogicFSM::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}
