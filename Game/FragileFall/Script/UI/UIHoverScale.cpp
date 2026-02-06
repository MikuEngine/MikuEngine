#include "GamePCH.h"
#include "UIHoverScale.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UIText.h>

namespace
{
    static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    static engine::UIText* FindUITextInChildren(engine::GameObject* root)
    {
        if (!root) return nullptr;

        engine::Transform* t = root->GetTransform();
        if (!t) return nullptr;

        for (engine::Transform* ct : t->GetChildren())
        {
            if (!ct) continue;

            engine::GameObject* childGO = ct->GetGameObject();
            if (!childGO) continue;

            // 1) 이 자식에 UIText가 있나?
            if (auto* txt = childGO->GetComponent<engine::UIText>())
                return txt;

            // 2) 없으면 더 아래(손자)로 내려감
            if (auto* txt2 = FindUITextInChildren(childGO))
                return txt2;
        }

        return nullptr;
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

        m_txt = FindUITextInChildren(go);
        if (m_txt)
        {
            m_baseFontSize = static_cast<float>(m_txt->GetFontPixelSize());
        }

        m_btn->AddOnHover(std::move([self = engine::Ptr<UIHoverScale>(this)](bool hovered)
            {
                if (!self) return;
                self->m_hovered = hovered; 
                self->Apply();

                if (!hovered)
                {
                    self->Reset();
                }
            }));
    }

    void UIHoverScale::Update()
    {

    }

    void UIHoverScale::OnGui()
    {
        ImGui::SliderFloat("HoverScale", &m_hoverScale, 1.0f, 1.5f, "%.2f");

        ImGui::Checkbox("use Text Bold", &m_useBold);

        ImGui::Checkbox("Always Bold", &m_alwaysBold);
    }

    void UIHoverScale::Save(engine::json& j) const
    {
        Object::Save(j);

        j["HoverScale"] = m_hoverScale;
        j["Speed"] = m_speed;
        j["Bold"] = m_useBold;
        j["AlwaysBold"] = m_alwaysBold;
    }

    void UIHoverScale::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "HoverScale", m_hoverScale);
        engine::JsonGet(j, "Speed", m_speed);
        engine::JsonGet(j, "Bold", m_useBold);
        engine::JsonGet(j, "AlwaysBold", m_alwaysBold);
    }

    void UIHoverScale::Apply()
    {
        if (!m_btn) return;

        const float mul = m_hovered ? m_hoverScale : 1.0f;
        const engine::Vector2 target = { m_baseSize.x * mul, m_baseSize.y * mul };

        m_btn->GetRectTransform()->SetSize(target.x, target.y);

        if (m_txt)
        {
            m_txt->SetFontPixelSize(static_cast<int>(m_baseFontSize * mul));

            if (m_useBold) m_txt->SetBold(true);
        }
    }

    void UIHoverScale::Reset()
    {
        if (m_txt && !m_alwaysBold)
        {
            m_txt->SetBold(false);
        }
    }
}