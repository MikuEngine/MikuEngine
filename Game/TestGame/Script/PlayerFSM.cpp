#include "GamePCH.h"
#include "PlayerFSM.h"
#include "AimPointer.h"
#include "TempBulletFactory.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

namespace game
{
    void PlayerFSM::Awake()
    {
        CharacterLogicFSM::Awake();
    }

    void PlayerFSM::Start()
    {
        CharacterLogicFSM::Start();
    }

    void PlayerFSM::Update()
    {
        CharacterLogicFSM::Update();
        
        // 추가: 발사 처리 (상태와 무관하게 마우스 클릭 시 발사)
        HandleShooting();
    }

    void PlayerFSM::CacheComponents()
    {
        CharacterLogicFSM::CacheComponents();
        
        // AimPointer와 BulletFactory 찾기
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* aimGO = scene->FindGameObject("AimPointer"))
            {
                m_aimPointer = aimGO->GetComponent<AimPointer>();
            }
            
            // BulletFactory는 같은 오브젝트 또는 씬에서 찾기
            m_bulletFactory = GetGameObject()->GetComponent<TempBulletFactory>();
            if (!m_bulletFactory)
            {
                if (auto* factoryGO = scene->FindGameObject("BulletFactory"))
                {
                    m_bulletFactory = factoryGO->GetComponent<TempBulletFactory>();
                }
            }
        }
    }

    bool PlayerFSM::IsAttackPressed() const
    {
        // 마우스 왼쪽 클릭으로 공격
        return engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
    }

    void PlayerFSM::OnEnterState(CharacterState state)
    {
        // 부모 처리 먼저
        CharacterLogicFSM::OnEnterState(state);
        
        // Attack 상태 진입 시 추가 처리 (필요시)
        if (state == CharacterState::Attack)
        {
            // 공격 시작 - 총알 발사는 HandleShooting에서 처리
        }
    }

    void PlayerFSM::UpdateCurrentState()
    {
        switch (m_currentState)
        {
        case CharacterState::Attack:
            // 공격 상태는 즉시 Idle로 복귀 (총알 발사는 순간 동작)
            ChangeState(CharacterState::Idle);
            break;
            
        default:
            // 나머지 상태는 부모 처리
            CharacterLogicFSM::UpdateCurrentState();
            break;
        }
    }

    void PlayerFSM::HandleShooting()
    {
        // 마우스 홀드 시 연속 발사
        if (engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT))
        {
            if (m_bulletFactory && m_aimPointer)
            {
                engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
                engine::Vector3 direction = m_aimPointer->GetDirectionFrom(playerPos);
                
                m_bulletFactory->Fire(playerPos, direction);
            }
        }
    }

    void PlayerFSM::OnGui()
    {
        CharacterLogicFSM::OnGui();
        
        ImGui::Separator();
        ImGui::Text("PlayerFSM References:");
        ImGui::Text("  AimPointer: %s", m_aimPointer ? "Found" : "NOT FOUND");
        ImGui::Text("  BulletFactory: %s", m_bulletFactory ? "Found" : "NOT FOUND");
    }

    void PlayerFSM::Save(engine::json& j) const
    {
        CharacterLogicFSM::Save(j);
    }

    void PlayerFSM::Load(const engine::json& j)
    {
        CharacterLogicFSM::Load(j);
    }
}
