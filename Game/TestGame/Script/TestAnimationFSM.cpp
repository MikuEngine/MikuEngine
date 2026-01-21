#include "GamePCH.h"
#include "TestAnimationFSM.h"
#include "TestLogicFSM.h"

#include <Framework/Object/Component/SkeletalAnimator.h>
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
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (animator)
        {
            animator->SetLayerWeight(1, 0.0f);
            animator->Play("TestIdle", true, 0, 1.0f);
        }
    }

    void TestAnimationFSM::Update()
    {
        // 부모 Update 호출하지 않음 (자체 처리)
        
        CharacterState currentState = GetCharacterState();
        
        // Test4 대기 중 처리
        if (m_isWaitingAfterTest4)
        {
            UpdateTest4Wait();
            return;
        }
        
        // 각 테스트 상태별 종료 체크
        switch (currentState)
        {
        case CharacterState::Test1:
            CheckTest1Finished();
            break;
        case CharacterState::Test2:
            CheckTest2EarlyExit();
            break;
        case CharacterState::Test3:
            CheckTest3Finished();
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
        
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;
        
        switch (context.currentState)
        {
        case CharacterState::Idle:
            // Idle: 상체 레이어 가중치 0으로, 기본 레이어에서 Idle 재생
            animator->SetLayerWeight(1, 0.0f);
            animator->PlayCrossFade("TestIdle", 0.2f, true, 0, 1.0f);
            break;
            
        case CharacterState::Test1:
            // Test1: 기본 재생
            animator->SetLayerWeight(1, 0.0f);
            animator->PlayCrossFade("TestForward", 0.1f, false, 0, 1.0f);
            break;
            
        case CharacterState::Test2:
            // Test2: 70%까지만 재생
            animator->SetLayerWeight(1, 0.0f);
            animator->PlayCrossFade("TestBackward", 0.1f, false, 0, 1.0f);
            break;
            
        case CharacterState::Test3:
            // Test3: 상체(펀치) + 하체(걷기) 분리
            // 레이어 0: 걷기 (전신, 상체 레이어가 덮어씌움)
            // 레이어 1: 펀치 (상체만, 마스크 적용 필요)
            m_test3Started = false;  // 애니메이션 시작 확인 플래그 리셋
            
            SetUpperBodyLayer(true, 1);

            animator->PlayCrossFade("TestForward", 0.1f, true, 0, 1.0f);  // 하체: 걷기 (루프)
            animator->SetLayerWeight(1, 1.0f);
            animator->PlayCrossFade("TestPunch", 0.1f, false, 1, 1.0f);   // 상체: 펀치 (루프 없음)
            break;
            
        case CharacterState::Test4:
            // Test4: 애니메이션 후 1초 대기
            animator->SetLayerWeight(1, 0.0f);
            animator->PlayCrossFade("TestElbow", 0.1f, false, 0, 1.0f);
            break;
            
        default:
            break;
        }
    }

    void TestAnimationFSM::OnStateExit(const StateContext& context)
    {
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;
        
        // Test3 종료 시 상체 레이어 가중치 리셋
        //if (context.currentState == CharacterState::Test3)
        //{
        //    animator->SetLayerWeight(1, 0.0f);
        //}
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
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;
        
        // 레이어 0에서 애니메이션 종료 체크 (NormalizedTime >= 0.9 사용)
        if (animator->GetNormalizedTime(0) >= 0.9f)
        {
            NotifyAnimationFinished(CharacterState::Test1);
        }
    }

    void TestAnimationFSM::CheckTest2EarlyExit()
    {
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;
        
        // 70% 진행 시 조기 종료
        if (animator->GetNormalizedTime(0) >= 0.7f)
        {
            NotifyAnimationFinished(CharacterState::Test2);
        }
    }

    void TestAnimationFSM::CheckTest3Finished()
    {
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;
        
        float normalizedTime = animator->GetNormalizedTime(1);
        
        // 애니메이션이 실제로 시작되었는지 확인 (이전 값이 남아있는 경우 방지)
        if (!m_test3Started)
        {
            // NormalizedTime이 0.1 미만이면 새로 시작된 것으로 판단
            if (normalizedTime < 0.1f)
            {
                m_test3Started = true;
            }
            return;  // 아직 시작 안됨, 체크 건너뛰기
        }
        
        // 애니메이션 종료 체크
        if (normalizedTime >= 0.9f)
        {
            NotifyAnimationFinished(CharacterState::Test3);
        }
    }

    void TestAnimationFSM::CheckTest4Finished()
    {
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;
        
        // 애니메이션 종료 후 대기 모드 진입 (NormalizedTime >= 0.9 사용)
        if (animator->GetNormalizedTime(0) >= 0.9f)
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
    }

    void TestAnimationFSM::Save(engine::json& j) const
    {
        Object::Save(j);
        j["Test4WaitDuration"] = m_test4WaitDuration;
    }

    void TestAnimationFSM::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "Test4WaitDuration", m_test4WaitDuration);
    }

    std::string TestAnimationFSM::GetType() const
    {
        return "TestAnimationFSM";
    }
}
