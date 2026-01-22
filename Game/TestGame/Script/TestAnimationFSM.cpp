#include "GamePCH.h"
#include "TestAnimationFSM.h"
#include "TestLogicFSM.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void TestAnimationFSM::Awake()
    {
        CharacterAnimationFSM::Awake();
        SetupAnimationMappings();
    }

    void TestAnimationFSM::Start()
    {
        CharacterAnimationFSM::Start();
        
        // 초기 Idle 애니메이션 즉시 재생 (T포즈 방지)
        if (m_animator)
        {
            m_animator->SetLayerWeight(1, 0.0f);
            m_animator->Play("TestIdle", true, 0, 1.0f);
        }
    }

    void TestAnimationFSM::Update()
    {
        // 부모 Update 호출 (레이어 체크 및 Procedural Aim)
        CharacterAnimationFSM::Update();
        
        CharacterState currentState = GetCharacterState();
        
        // Test4 대기 중 처리
        if (m_isWaitingAfterTest4)
        {
            UpdateTest4Wait();
            return;
        }
        
        // 각 테스트 상태별 종료 체크 (Test3은 OnUpperLayerFinished에서 처리)
        switch (currentState)
        {
        case CharacterState::Test1:
            CheckTest1Finished();
            break;
        case CharacterState::Test2:
            CheckTest2EarlyExit();
            break;
        case CharacterState::Test3:
            // Test3: 부모 클래스의 CheckLayerAnimationFinished()에서 체크
            // OnUpperLayerFinished() 콜백에서 처리
            break;
        case CharacterState::Test4:
            CheckTest4Finished();
            break;
        default:
            break;
        }
    }

    void TestAnimationFSM::OnStateEnter(const StateContext& context)
    {
        m_currentAnimState = context.currentState;

        // Test4 대기 상태 리셋
        m_isWaitingAfterTest4 = false;
        m_test4WaitTimer = 0.0f;
        
        if (!m_animator) return;
        
        switch (context.currentState)
        {
        case CharacterState::Idle:
            // Idle: 상체 레이어 가중치 0으로, 기본 레이어에서 Idle 재생
            m_animator->SetLayerWeight(1, 0.0f);
            m_animator->PlayCrossFade("TestIdle", 0.2f, true, 0, 1.0f);
            m_baseLayerState.Reset();
            m_upperLayerState.Reset();
            break;
            
        case CharacterState::Test1:
            // Test1: 기본 재생
            m_animator->SetLayerWeight(1, 0.0f);
            m_animator->PlayCrossFade("TestForward", 0.1f, false, 0, 1.0f);
            break;
            
        case CharacterState::Test2:
            // Test2: 70%까지만 재생
            m_animator->SetLayerWeight(1, 0.0f);
            m_animator->PlayCrossFade("TestBackward", 0.1f, false, 0, 1.0f);
            break;
            
        case CharacterState::Test3:
            // Test3: 상체(펀치) + 하체(걷기) 분리 - 새 인터페이스 사용
            PlaySplitAnimation(
                "TestForward", true,   // 하체: 걷기 (루프)
                "TestPunch", false,    // 상체: 펀치 (비루프)
                0.1f
            );
            SetUpperLayerExitCondition(0.95f);  // 상체 95%에서 종료 판정
            break;
            
        case CharacterState::Test4:
            // Test4: 애니메이션 후 1초 대기
            m_animator->SetLayerWeight(1, 0.0f);
            m_animator->PlayCrossFade("TestElbow", 0.1f, false, 0, 1.0f);
            break;
            
        default:
            break;
        }
    }

    void TestAnimationFSM::OnStateExit(const StateContext& context)
    {
        // 부모 호출 (레이어 상태 리셋)
        CharacterAnimationFSM::OnStateExit(context);
        
        if (!m_animator) return;
        
        // Test3 종료 시 상체 레이어 완전 리셋
        if (context.currentState == CharacterState::Test3)
        {
            m_animator->SetLayerWeight(1, 0.0f);
            m_animator->Play("TestIdle", true, 1, 1.0f);
        }
    }

    void TestAnimationFSM::OnUpperLayerFinished()
    {
        // Test3 상체 애니메이션 종료 시 콜백
        if (GetCharacterState() == CharacterState::Test3)
        {
            // 상체 레이어 비활성화
            if (m_animator)
            {
                m_animator->SetLayerWeight(m_upperBodyLayerIndex, 0.0f);
            }
            
            // 로직 FSM에 종료 알림
            NotifyAnimationFinished(CharacterState::Test3);
        }
    }

    void TestAnimationFSM::SetupAnimationMappings()
    {
        // 기본 매핑 (OnStateEnter에서 직접 처리하지만 참조용으로 설정)
        SetStateAnimation(CharacterState::Idle, "TestIdle", 0.2f, true, 1.0f);
        SetStateAnimation(CharacterState::Test1, "TestForward", 0.1f, false, 1.0f);
        SetStateAnimation(CharacterState::Test2, "TestBackward", 0.1f, false, 1.0f);
        SetStateAnimation(CharacterState::Test3, "TestPunch", 0.1f, false, 1.0f);
        SetStateAnimation(CharacterState::Test4, "TestElbow", 0.1f, false, 1.0f);
        
        // 상체 분리 모드 활성화
        SetUpperBodyLayer(true, 1);
    }

    void TestAnimationFSM::CheckTest1Finished()
    {
        if (!m_animator) return;
        
        if (m_animator->GetNormalizedTime(0) >= 0.9f)
        {
            NotifyAnimationFinished(CharacterState::Test1);
        }
    }

    void TestAnimationFSM::CheckTest2EarlyExit()
    {
        if (!m_animator) return;
        
        // 70% 진행 시 조기 종료
        if (m_animator->GetNormalizedTime(0) >= 0.7f)
        {
            NotifyAnimationFinished(CharacterState::Test2);
        }
    }

    void TestAnimationFSM::CheckTest4Finished()
    {
        if (!m_animator) return;
        
        // 애니메이션 종료 후 대기 모드 진입
        if (m_animator->GetNormalizedTime(0) >= 0.9f)
        {
            m_isWaitingAfterTest4 = true;
            m_test4WaitTimer = 0.0f;
        }
    }

    void TestAnimationFSM::UpdateTest4Wait()
    {
        m_test4WaitTimer += engine::Time::DeltaTime();
        
        // 1초 대기 후 Idle로 복귀
        if (m_test4WaitTimer >= m_test4WaitDuration)
        {
            m_isWaitingAfterTest4 = false;
            NotifyAnimationFinished(CharacterState::Test4);
        }
    }

    void TestAnimationFSM::OnGui()
    {
        ImGui::Text("TestAnimationFSM");
        ImGui::Text("Current State: %s", CharacterStateToString(GetCharacterState()));
        
        if (m_isWaitingAfterTest4)
        {
            ImGui::Text("Test4 Wait: %.2f / %.2f", m_test4WaitTimer, m_test4WaitDuration);
        }
        
        ImGui::Separator();
        ImGui::Text("Animation Mappings:");
        ImGui::Text("  Test1 -> TestForward (ends -> Idle)");
        ImGui::Text("  Test2 -> TestBackward (70%% -> Idle)");
        ImGui::Text("  Test3 -> Walk + Punch (upper/lower split)");
        ImGui::Text("  Test4 -> TestElbow (ends + 1s wait -> Idle)");
        
        ImGui::Separator();
        ImGui::DragFloat("Test4 Wait Duration", &m_test4WaitDuration, 0.1f, 0.0f, 5.0f);
        
        // 부모 GUI도 표시
        ImGui::Separator();
        CharacterAnimationFSM::OnGui();
    }

    void TestAnimationFSM::Save(engine::json& j) const
    {
        CharacterAnimationFSM::Save(j);
        j["Test4WaitDuration"] = m_test4WaitDuration;
    }

    void TestAnimationFSM::Load(const engine::json& j)
    {
        CharacterAnimationFSM::Load(j);
        engine::JsonGet(j, "Test4WaitDuration", m_test4WaitDuration);
    }
}
