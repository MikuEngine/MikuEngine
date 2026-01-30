#include "GamePCH.h"
#include "UIPopUpAnimator.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Core/System/MyTime.h>

namespace game
{
    static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    void UIPopUpAnimator::Awake()
    {

    }

    void UIPopUpAnimator::Start()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_rt = go->GetComponent<engine::RectTransform>();
        if (!m_rt) return;

        if (!m_inited)
        {
            m_visible = m_rt->GetAnchoredPosition();
            m_hidden = { m_visible.x, m_visible.y + 700.0f };
            m_inited = true;
        }
    }

    void UIPopUpAnimator::Update()
    {
        if (!m_animating || !m_rt) return;

        m_time += engine::Time::UnscaledDeltaTime();
        float t = m_time / m_duration;
        if (t > 1.0f) t = 1.0f;

        if (m_opening)
        {
            m_rt->SetAnchoredPosition(Lerp(m_hidden, m_visible, t));
        }
        else
        {
            m_rt->SetAnchoredPosition(Lerp(m_visible, m_hidden, t));
        }

        if (t >= 1.0f)
        {
            m_animating = false;

            m_rt->SetAnchoredPosition(m_isOpen ? m_visible : m_hidden);

            if (!m_isOpen)
            {
                GetGameObject()->SetActive(false);
            }
        }
    }

    void UIPopUpAnimator::OnGui()
    {
        ImGui::DragFloat("Duration", &m_duration);
    }

    void UIPopUpAnimator::Save(engine::json& j) const
    {
        Object::Save(j);
        j["Duration"] = m_duration;
        j["ClosedOffsetY"] = m_visible.y - m_hidden.y;
    }

    void UIPopUpAnimator::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "Duration", m_duration);
    }

    void UIPopUpAnimator::Open()
    {
        auto* go = GetGameObject();
        if (!go) return;
        go->SetActive(true);;

        if (!m_rt)
        {
            m_rt = go->GetComponent<engine::RectTransform>();
            if (!m_rt) return;
        }

        if (!m_inited)
        {
            m_visible = m_rt->GetAnchoredPosition();
            m_hidden = { m_visible.x, m_visible.y + 700.0f };
            m_inited = true;
        }

        m_isOpen = true;
        m_opening = true;
        m_animating = true;
        m_time = 0.0f;

        m_rt->SetAnchoredPosition(m_hidden);
    }

    void UIPopUpAnimator::Close()
    {
        if (!m_isOpen) return;
        if (!m_rt) return;

        m_isOpen = false;
        m_opening = false;
        m_animating = true;
        m_time = 0.0f;
    }
}