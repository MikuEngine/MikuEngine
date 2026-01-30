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
        auto* go = GetGameObject();
        if (!go) return;

        go->SetActive(true);

        if (!m_rt)
        {
            m_rt = go->GetComponent<engine::RectTransform>();
            if (!m_rt) return;
        }

        if (!m_inited)
        {
            CacheTargets();
            m_inited = true;
        }

        m_finished = false;

        m_toPos = targetPos;
        m_fromPos = { targetPos.x + m_enterSlideOffsetX, targetPos.y };

        m_duration = m_enterDuration;
        m_time = 0.0f;
        m_animating = true;

        // 들어올 때는 알파 원복
        for (auto& trg : m_alphaTargets)
        {
            if (trg.text)
            {
                auto c = trg.text->GetColor();
                c.w = trg.baseA;
                trg.text->SetColor(c);
            }
            if (trg.image)
            {
                auto c = trg.image->GetColor();
                c.w = trg.baseA;
                trg.image->SetColor(c);
            }
        }

        m_rt->SetAnchoredPosition(m_fromPos);
    }

    void UIToastAnimator::MoveTo(const engine::Vector2& targetPos, float durationOverride)
    {
        if (!m_rt)  return;

        m_finished = false;

        m_fromPos = m_rt->GetAnchoredPosition();
        m_toPos = targetPos;

        m_duration = (durationOverride > 0.0f) ? durationOverride : m_moveDuration;
        m_time = 0.0f;
        m_animating = true;
    }

    void UIToastAnimator::PlayFadeOut(float durationOverride)
    {
        if (m_finished) return;
        if (m_fading) return;

        if (!m_inited)
        {
            CacheTargets();
            m_inited = true;
        }

        // 페이드 시작
        m_fading = true;
        m_finished = false;
        m_fadeTime = 0.0f;

        if (durationOverride > 0.0f)
            m_fadeDuration = durationOverride;

        // 혹시 이전에 알파가 0에 가까웠던 상태를 대비해, 시작 알파를 원복하고 시작
        for (auto& trg : m_alphaTargets)
        {
            if (trg.text)
            {
                auto c = trg.text->GetColor();
                c.w = trg.baseA;
                trg.text->SetColor(c);
            }
            if (trg.image)
            {
                auto c = trg.image->GetColor();
                c.w = trg.baseA;
                trg.image->SetColor(c);
            }
        }
    }

    void UIToastAnimator::SetText(const std::string& text)
    {
        auto* go = GetGameObject();
        if (!go) return;

        // 자식에서 첫 UIText 찾기(DFS)
        auto* root = go->GetTransform();
        if (!root) return;

        std::vector<engine::Transform*> stack;
        stack.push_back(root);

        while (!stack.empty())
        {
            engine::Transform* cur = stack.back();
            stack.pop_back();

            if (auto* cgo = cur->GetGameObject())
            {
                if (auto* t = cgo->GetComponent<engine::UIText>())
                {
                    t->SetText(text);
                    return;
                }
            }

            for (auto* ch : cur->GetChildren())
                stack.push_back(ch);
        }
    }

    void UIToastAnimator::CacheTargets()
    {
        m_alphaTargets.clear();

        auto* go = GetGameObject();
        if (!go) return;

        auto* tr = go->GetTransform();
        if (!tr) return;

        CaptureBaseAlphaRecursive(tr);
    }

    void UIToastAnimator::SetAlphaRecursive(engine::Transform* t, float a)
    {
        if (!t) return;

        auto* go = t->GetGameObject();
        if (go)
        {
            if (auto* text = go->GetComponent<engine::UIText>())
            {
                auto c = text->GetColor();
                c.w = a;
                text->SetColor(c);
            }

            if (auto* img = go->GetComponent<engine::UIImage>())
            {
                auto c = img->GetColor();
                c.w = a;
                img->SetColor(c);
            }
        }

        for (auto* c : t->GetChildren())
            SetAlphaRecursive(c, a);
    }

    void UIToastAnimator::CaptureBaseAlphaRecursive(engine::Transform* t)
    {
        if (!t) return;

        auto* go = t->GetGameObject();
        if (go)
        {
            if (auto* text = go->GetComponent<engine::UIText>())
            {
                AlphaTarget at;
                at.text = text;
                at.baseA = text->GetColor().w;
                m_alphaTargets.push_back(at);
            }

            if (auto* img = go->GetComponent<engine::UIImage>())
            {
                AlphaTarget at;
                at.image = img;
                at.baseA = img->GetColor().w;
                m_alphaTargets.push_back(at);
            }
        }

        for (auto* c : t->GetChildren())
            CaptureBaseAlphaRecursive(c);
    }
}