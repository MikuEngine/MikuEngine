#include "GamePCH.h"
#include "DoorTriggerScript.h"

#include <Framework/Scene/SceneManager.h>

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/BoxCollider.h>

#include <Framework/Object/Component/Renderer/DebugRenderer.h>

namespace game
{
    void DoorTriggerScript::Awake()
    {
        m_doorPosition = GetTransform()->GetWorldPosition();
    }

    void DoorTriggerScript::Start()
    {
        SetActivateDoor(m_isActive);
    }

    void DoorTriggerScript::Update()
    {
#ifdef _DEBUG
        if (m_isActive)
        {
            engine::DebugRenderer::Get().AddDebugCircle(
                m_doorPosition,
                6.0f,
                engine::Vector3::UnitY,
                DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.2f),
                32
            );
        }
#endif
    }

    void DoorTriggerScript::SetActivateDoor(bool active)
    {
        m_isActive = active;

        // 현재 Collision Active false가 안되서 로직을 넣음
        if (m_isActive)
        {
            auto* collider = GetGameObject()->GetComponent<engine::Collider>();
            if (!collider)
            {
                auto* newCollider = GetGameObject()->AddComponent<engine::BoxCollider>();

                newCollider->SetCenter(engine::Vector3(0.0f, 3.0f, 0.0f));
                newCollider->SetSize(engine::Vector3(7.0f, 8.0f, 2.0f));
            }
        }
        else
        {
            auto* collider = GetGameObject()->GetComponent<engine::Collider>();
            if (collider)
            {
                collider->Destroy();
            }
        }
    }

    void DoorTriggerScript::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (info.gameObject->GetName() == "Player")
        {
            // 이벤트 콜백이 있다면 실행
            if (m_onTriggered)
            {
                m_onTriggered();
            }

            if (!m_nextSceneName.empty())
            {
                engine::SceneManager::Get().ChangeScene(m_nextSceneName);
            }
        }
    }

    void DoorTriggerScript::OnGui()
    {
        char sceneNameBuffer[256];
        strcpy_s(sceneNameBuffer, m_nextSceneName.c_str());

        if (ImGui::InputText("Next Scene Name", sceneNameBuffer, sizeof(sceneNameBuffer)))
        {
            m_nextSceneName = sceneNameBuffer;
        }
    }

    void DoorTriggerScript::Save(engine::json& j) const
    {
        Object::Save(j);

        j["NextSceneName"] = m_nextSceneName;
    }

    void DoorTriggerScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "NextSceneName", m_nextSceneName);
    }
}