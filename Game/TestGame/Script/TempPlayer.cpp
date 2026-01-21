#include "GamePCH.h"
#include "TempPlayer.h"
#include "AimPointer.h"
#include "TempBulletFactory.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

namespace game
{
    void TempPlayer::Start()
    {
        LOG_PRINT("[TempPlayer] Started");
        
        // 씬에서 AimPointer 찾기
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* aimGO = scene->FindGameObject("AimPointer"))
            {
                m_aimPointer = aimGO->GetComponent<AimPointer>();
                if (m_aimPointer)
                {
                    LOG_PRINT("[TempPlayer] Found AimPointer");
                }
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
            
            if (m_bulletFactory)
            {
                LOG_PRINT("[TempPlayer] Found TempBulletFactory");
            }
        }

        if (!m_aimPointer)
        {
            LOG_INFO("[TempPlayer] Warning: AimPointer not found! Create a GameObject named 'AimPointer' with AimPointer script.");
        }
        if (!m_bulletFactory)
        {
            LOG_INFO("[TempPlayer] Warning: TempBulletFactory not found! Add TempBulletFactory component to this object or create 'BulletFactory' GameObject.");
        }
    }

    void TempPlayer::Update()
    {
        HandleMovement();
        HandleShooting();
    }

    void TempPlayer::HandleMovement()
    {
        engine::Vector3 movement(0.0f, 0.0f, 0.0f);
        
        // 화살표 키 입력
        if (engine::Input::IsKeyHeld(engine::Keys::Up) )
        {
            movement.y += 1.0f;
        }
        if (engine::Input::IsKeyHeld(engine::Keys::Down) )
        {
            movement.y -= 1.0f;
        }
        if (engine::Input::IsKeyHeld(engine::Keys::Left) )
        {
            movement.x -= 1.0f;
        }
        if (engine::Input::IsKeyHeld(engine::Keys::Right) )
        {
            movement.x += 1.0f;
        }

        // 이동 적용 (정규화 후 속도 적용)
        if (movement.LengthSquared() > 0.0f)
        {
            movement.Normalize();
            engine::Vector3 currentPos = GetTransform()->GetLocalPosition();
            currentPos += movement * m_moveSpeed * engine::Time::DeltaTime();
            GetTransform()->SetLocalPosition(currentPos);
        }
    }

    void TempPlayer::HandleShooting()
    {
        // 스페이스바로 발사
        if (engine::Input::IsKeyHeld(engine::Keys::Space))
        {
            if (m_bulletFactory && m_aimPointer)
            {
                engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
                engine::Vector3 direction = m_aimPointer->GetDirectionFrom(playerPos);
                
                m_bulletFactory->Fire(playerPos, direction);
            }
        }
    }

    void TempPlayer::OnGui()
    {
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.1f, 20.0f);
        
        ImGui::Separator();
        ImGui::Text("References:");
        ImGui::Text("  AimPointer: %s", m_aimPointer ? "Found" : "NOT FOUND");
        ImGui::Text("  BulletFactory: %s", m_bulletFactory ? "Found" : "NOT FOUND");
    }

    void TempPlayer::Save(engine::json& j) const
    {
        Object::Save(j);
        j["MoveSpeed"] = m_moveSpeed;
    }

    void TempPlayer::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "MoveSpeed", m_moveSpeed);
    }

    std::string TempPlayer::GetType() const
    {
        return "TempPlayer";
    }
}
