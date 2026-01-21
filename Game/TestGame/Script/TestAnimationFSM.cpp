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
        LOG_PRINT("[TestAnimationFSM] Started");
    }

    void TestAnimationFSM::Update()
    {
        CharacterAnimationFSM::Update();

        // 공격 타이머 체크
        if (GetCharacterState() == CharacterState::Attack)
        {
            m_attackTimer += engine::Time::DeltaTime();
            
            if (m_attackTimer >= m_attackDuration)
            {
                NotifyAnimationFinished(CharacterState::Attack);
                m_attackTimer = 0.0f;
            }
        }
    }

    void TestAnimationFSM::OnStateEnter(const StateContext& context)
    {
        CharacterAnimationFSM::OnStateEnter(context);

        // 공격 진입 시 타이머 리셋
        if (context.currentState == CharacterState::Attack)
        {
            m_attackTimer = 0.0f;
        }
    }

    void TestAnimationFSM::OnStateUpdate(const StateContext& context)
    {
        CharacterAnimationFSM::OnStateUpdate(context);
        
        // 이동 중일 때 이동 방향에 따라 Forward/Backward 전환 가능
        // (현재는 단순히 Forward만 사용)
    }

    void TestAnimationFSM::SetupAnimationMappings()
    {
        // TestBot 모델의 애니메이션 매핑
        // 애니메이션 이름은 SkeletalAnimator에 등록된 이름과 일치해야 함
        
        SetStateAnimation(CharacterState::Idle, "TestIdle", 0.2f, true, 1.0f);
        SetStateAnimation(CharacterState::Walk, "TestForward", 0.2f, true, 1.0f);
        SetStateAnimation(CharacterState::Attack, "TestPunch", 0.1f, false, 1.0f);
        
        // 상체 분리 모드 (옵션)
        SetUpperBodyLayer(true, 1);
        SetUpperBodyWeight(1.0f);
    }

    void TestAnimationFSM::OnGui()
    {
        CharacterAnimationFSM::OnGui();

        ImGui::Separator();
        ImGui::Text("TestAnimationFSM Settings");
        
        ImGui::DragFloat("Attack Duration", &m_attackDuration, 0.01f, 0.1f, 3.0f);
        
        if (GetCharacterState() == CharacterState::Attack)
        {
            ImGui::Text("Attack Timer: %.2f / %.2f", m_attackTimer, m_attackDuration);
        }
    }

    void TestAnimationFSM::Save(engine::json& j) const
    {
        CharacterAnimationFSM::Save(j);
        
        j["AttackDuration"] = m_attackDuration;
    }

    void TestAnimationFSM::Load(const engine::json& j)
    {
        CharacterAnimationFSM::Load(j);
        
        engine::JsonGet(j, "AttackDuration", m_attackDuration);
    }

    std::string TestAnimationFSM::GetType() const
    {
        return "TestAnimationFSM";
    }
}
