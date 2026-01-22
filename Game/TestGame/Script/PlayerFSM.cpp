#include "GamePCH.h"
#include "PlayerFSM.h"
#include "AimPointer.h"
#include "TempBulletFactory.h"
#include "CharacterAnimationFSM.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Engine/Core/System/Input.h>

namespace game
{
    void PlayerFSM::Awake()
    {
        CharacterLogicFSM::Awake();
    }

    void PlayerFSM::Start()
    {
        CharacterLogicFSM::Start();
        
        // Procedural Aim 활성화
        if (m_charAnimFSM && m_enableUpperBodyAim)
        {
            m_charAnimFSM->SetProceduralAimEnabled(true);
        }
    }

    void PlayerFSM::CacheComponents()
    {
        CharacterLogicFSM::CacheComponents();
        
        // CharacterAnimationFSM 찾기
        m_charAnimFSM = GetGameObject()->GetComponent<CharacterAnimationFSM>();
        
        // AimPointer와 BulletFactory 찾기
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* aimGO = scene->FindGameObject("AimPointer"))
            {
                m_aimPointer = aimGO->GetComponent<AimPointer>();
            }
            
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

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 - 모든 플레이어 입력을 여기서 context에 설정
    // ═══════════════════════════════════════════════════════════════
    void PlayerFSM::ProcessInput()
    {
        // 1. 이동 입력 → context
        m_context.moveDirection = GetMoveInputDirection();
        
        // 2. 공격 입력 → context
        m_context.attackPressed = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
        m_context.attackHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);
        
        // 3. 상호작용 입력 → context
        m_context.interactPressed = engine::Input::IsKeyPressed(engine::Keys::E);
        
        // 4. 상체 조준 업데이트 (매 프레임)
        UpdateUpperBodyAim();
    }

    void PlayerFSM::OnEnterState(CharacterState state)
    {
        CharacterLogicFSM::OnEnterState(state);
    }

    void PlayerFSM::UpdateCurrentState()
    {
        switch (m_currentState)
        {
        case CharacterState::Idle:
        case CharacterState::Walk:
            // 부모 처리 (이동, 상태 전이)
            CharacterLogicFSM::UpdateCurrentState();
            
            // 연속 발사 (attackHeld 사용)
            if (m_context.attackHeld)
            {
                HandleShooting();
            }
            break;
            
        case CharacterState::Attack:
            // 공격 상태는 즉시 Idle로 복귀 (총알 발사는 순간 동작)
            ChangeState(CharacterState::Idle);
            break;
            
        default:
            CharacterLogicFSM::UpdateCurrentState();
            break;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 전용 입력 함수 (private)
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 PlayerFSM::GetMoveInputDirection() const
    {
        engine::Vector3 direction = engine::Vector3::Zero;

        if (engine::Input::IsKeyHeld(engine::Keys::W)) direction.y += 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::S)) direction.y -= 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::A)) direction.x -= 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::D)) direction.x += 1.0f;

        if (direction.LengthSquared() > 0.0f)
        {
            direction.Normalize();
        }

        return direction;
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 전용 액션
    // ═══════════════════════════════════════════════════════════════
    void PlayerFSM::HandleShooting()
    {
        if (m_bulletFactory && m_aimPointer)
        {
            engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
            engine::Vector3 direction = m_aimPointer->GetDirectionFrom(playerPos);
            
            m_bulletFactory->Fire(playerPos, direction);
        }
    }

    void PlayerFSM::UpdateUpperBodyAim()
    {
        if (!m_enableUpperBodyAim || !m_charAnimFSM || !m_aimPointer)
        {
            return;
        }
        
        float yaw = CalculateAimYaw();
        m_charAnimFSM->SetUpperBodyYaw(yaw);
    }

    float PlayerFSM::CalculateAimYaw() const
    {
        if (!m_aimPointer)
        {
            return 0.0f;
        }
        
        engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
        engine::Vector3 aimPos = m_aimPointer->GetTransform()->GetWorldPosition();
        
        engine::Vector3 toAim = aimPos - playerPos;
        toAim.y = 0.0f;
        
        if (toAim.LengthSquared() < 0.001f)
        {
            return 0.0f;
        }
        
        toAim.Normalize();
        
        // 플레이어 전방 (+Z)
        engine::Vector3 forward = engine::Vector3::UnitZ;
        engine::Quaternion playerRot = GetTransform()->GetWorldRotation();
        forward = engine::Vector3::Transform(forward, playerRot);
        forward.y = 0.0f;
        forward.Normalize();
        
        // 상대 각도 계산
        float dotProduct = forward.Dot(toAim);
        engine::Vector3 crossProduct = forward.Cross(toAim);
        
        float angleRad = atan2f(crossProduct.y, dotProduct);
        float angleDeg = engine::ToDegree(angleRad);
        
        return angleDeg;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void PlayerFSM::OnGui()
    {
        CharacterLogicFSM::OnGui();
        
        ImGui::Separator();
        ImGui::Text("PlayerFSM:");
        ImGui::Checkbox("Enable Upper Body Aim", &m_enableUpperBodyAim);
        
        // 입력 상태 표시
        ImGui::Text("Input:");
        ImGui::Text("  Attack Pressed: %s", m_context.attackPressed ? "YES" : "no");
        ImGui::Text("  Attack Held: %s", m_context.attackHeld ? "YES" : "no");
        ImGui::Text("  Interact: %s", m_context.interactPressed ? "YES" : "no");
        
        ImGui::Separator();
        ImGui::Text("References:");
        ImGui::Text("  AimPointer: %s", m_aimPointer ? "Found" : "NOT FOUND");
        ImGui::Text("  BulletFactory: %s", m_bulletFactory ? "Found" : "NOT FOUND");
        ImGui::Text("  AnimFSM: %s", m_charAnimFSM ? "Found" : "NOT FOUND");
        
        if (m_aimPointer)
        {
            float yaw = CalculateAimYaw();
            ImGui::Text("  Aim Yaw: %.1f deg", yaw);
        }
    }

    void PlayerFSM::Save(engine::json& j) const
    {
        CharacterLogicFSM::Save(j);
        j["EnableUpperBodyAim"] = m_enableUpperBodyAim;
    }

    void PlayerFSM::Load(const engine::json& j)
    {
        CharacterLogicFSM::Load(j);
        engine::JsonGet(j, "EnableUpperBodyAim", m_enableUpperBodyAim);
    }
}
