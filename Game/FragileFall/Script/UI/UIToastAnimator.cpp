#include "GamePCH.h"
#include "UIToastAnimator.h"

#include <Framework/Object/Component/RectTransform.h>

namespace game
{
    engine::Vector2 UIToastAnimator::Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    void UIToastAnimator::Awake()
    {

    }

    void UIToastAnimator::Start()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_rt = go->GetComponent<engine::RectTransform>();
        if (!m_rt) return;

        if (!m_inited)
        {
            CacheTargets();
            m_inited = true;
        }
    }

    void UIToastAnimator::Update()
    {
        if (!m_rt) return;

        if (m_animating)
        {
            m_time += engine::Time::UnscaledDeltaTime();
            float t = (m_duration > 0.0f) ? (m_time / m_duration) : 1.0f;
            if (t > 1.0f) t = 1.0f;

            m_rt->SetAnchoredPosition(Lerp(m_fromPos, m_toPos, t));

            if (t >= 1.0f)
            {
                m_animating = false;
                m_rt->SetAnchoredPosition(m_toPos);
            }
        }

        if (m_fading)
        {
            m_fadeTime += engine::Time::UnscaledDeltaTime();
            float t = (m_fadeDuration > 0.0f) ? (m_fadeTime / m_fadeDuration) : 1.0f;
            if (t > 1.0f) t = 1.0f;

            const float a = 1.0f - t;

            // baseA를 곱해서 원래 알파 비율 유지
            for (auto& trg : m_alphaTargets)
            {
                if (trg.text)
                {
                    auto c = trg.text->GetColor();
                    c.w = trg.baseA * a;
                    trg.text->SetColor(c);
                }
                if (trg.image)
                {
                    auto c = trg.image->GetColor();
                    c.w = trg.baseA * a;
                    trg.image->SetColor(c);
                }
            }

            if (t >= 1.0f)
            {
                m_fading = false;
                m_finished = true;
            }
        }
    }

    void UIToastAnimator::OnGui()
    {
        ImGui::DragFloat("Enter Slide Offset X", &m_enterSlideOffsetX, 1.0f, 0.0f, 2000.0f);
        ImGui::DragFloat("Enter Duration", &m_enterDuration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Move Duration", &m_moveDuration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Fade Duration", &m_fadeDuration, 0.01f, 0.01f, 2.0f);
    }

    void UIToastAnimator::Save(engine::json& j) const
    {
        Object::Save(j);
        j["EnterSlideOffsetX"] = m_enterSlideOffsetX;
        j["EnterDuration"] = m_enterDuration;
        j["MoveDuration"] = m_moveDuration;
        j["FadeDuration"] = m_fadeDuration;
    }

    void UIToastAnimator::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "EnterSlideOffsetX", m_enterSlideOffsetX);
        engine::JsonGet(j, "EnterDuration", m_enterDuration);
        engine::JsonGet(j, "MoveDuration", m_moveDuration);
        engine::JsonGet(j, "FadeDuration", m_fadeDuration);
    }

    void UIToastAnimator::PlayEnter(const engine::Vector2& targetPos)
    {

    }

    void UIToastAnimator::MoveTo(const engine::Vector2& targetPos, float durationOverride)
    {

    }

    void UIToastAnimator::PlayFadeOut(float durationOverride)
    {

    }
    void UIToastAnimator::SetText(const std::string& text)
    {

    }

    void UIToastAnimator::CacheTargets()
    {

    }

    void UIToastAnimator::SetAlphaRecursive(engine::Transform* t, float a)
    {

    }

    void UIToastAnimator::CaptureBaseAlphaRecursive(engine::Transform* t)
    {
    }
}