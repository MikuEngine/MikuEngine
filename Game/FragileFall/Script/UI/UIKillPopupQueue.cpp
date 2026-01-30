#include "GamePCH.h"
#include "UIKillPopupQueue.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Scene/Scene.h>

#include <Framework/Object/Component/UI/UIPanel.h>
#include <Framework/Object/Component/UI/UIImage.h>

namespace game
{
    static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    void UIKillPopupQueue::Awake()
    {
        m_canvas = engine::GameObject::Find("Canvas_KillPopUp");
        if (!m_canvas) return;
    }

    void UIKillPopupQueue::Start()
    {

    }

    void UIKillPopupQueue::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::K))
        {
            m_container = CreateGameObject(engine::CreateObjectType::UI, "KillPopup");
            m_container->GetTransform()->SetParent(m_canvas->GetTransform());

            m_container->AddComponent<engine::UIPanel>();
        }
    }

    void UIKillPopupQueue::OnGui()
    {

    }

    void UIKillPopupQueue::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void UIKillPopupQueue::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}