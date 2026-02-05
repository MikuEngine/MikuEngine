#include "GamePCH.h"
#include "LobbyInteraction.h"
#include <Framework/Object/Component/UI/UIClickArea.h>

namespace game
{
    void LobbyInteraction::Awake()
    {
        m_player = engine::GameObject::Find("PlayerPreview");
        auto* go = GetGameObject();
        if (go) m_clickArea = go->GetComponent<engine::UIClickArea>();
    }

    void LobbyInteraction::Start()
    {
        if (!m_clickArea || !m_player) return;

        const auto& rot = m_player->GetTransform()->GetLocalEulerAngles(); // Vector3 반환 함수가 필요합니다.
        m_yawDeg = rot.y;

        m_clickArea->AddOnDrag([self = engine::Ptr<LobbyInteraction>(this)](const engine::Vector2& /*pos*/, const engine::Vector2& delta, int mouseButton)
            {
                if (!self) return;
                if (mouseButton != 0) return;

                self->Interact(delta);
            });
    }

    void LobbyInteraction::Update()
    {
        
    }

    void LobbyInteraction::OnGui()
    {

    }

    void LobbyInteraction::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void LobbyInteraction::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void LobbyInteraction::Interact(const engine::Vector2& delta)
    {
        float degPerPixel = 0.5f;

        m_yawDeg -= delta.x * degPerPixel;

        m_player->GetTransform()->SetLocalRotation({ 0.0f, m_yawDeg, 0.0f });
    }
}