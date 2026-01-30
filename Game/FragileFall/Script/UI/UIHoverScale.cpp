#include "GamePCH.h"
#include "UIHoverScale.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace
{
    static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }
}

namespace game
{
    void UIHoverScale::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_rt = go->GetComponent<engine::RectTransform>();
        if (!m_rt) return;

        m_btn = go->GetComponent<engine::UIButton>();
        if (!m_btn) return;

        m_baseSize = m_btn->GetRectTransform()->GetSize();

        m_btn->AddOnHover(std::move([self = engine::Ptr<UIHoverScale>(this)](bool hovered)
            {
                if (!self) return;
                self->m_hovered = hovered; 
                self->Apply();
            }));
    }

    void UIHoverScale::Update()
    {

    }

    void UIHoverScale::OnGui()
    {
        ImGui::SliderFloat("HoverScale", &m_hoverScale, 1.0f, 1.5f, "%.2f");
    }

    void UIHoverScale::Save(engine::json& j) const
    {
        Object::Save(j);

        j["HoverScale"] = m_hoverScale;
        j["Speed"] = m_speed;
    }

    void UIHoverScale::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "HoverScale", m_hoverScale);
        engine::JsonGet(j, "Speed", m_speed);
    }

    void UIHoverScale::Apply()
    {
        //if (!m_rt) return;
        if (!m_btn) return;

        const float mul = m_hovered ? m_hoverScale : 1.0f;
        const engine::Vector2 target = { m_baseSize.x * mul, m_baseSize.y * mul };

        m_btn->GetRectTransform()->SetSize(target.x, target.y);

        //m_rt->SetSize(target.x, target.y);
    }
}