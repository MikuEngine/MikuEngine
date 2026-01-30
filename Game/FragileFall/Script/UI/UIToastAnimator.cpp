#include "GamePCH.h"
#include "UIToastAnimator.h"

#include <Framework/Object/Component/RectTransform.h>

namespace game
{
    engine::Vector2 UIToastAnimator::Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    static float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
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
            CacheTargets();       // baseA 확보
            m_inited = true;
        }

        // 기본 상태: 완전히 보이게(원하시면 0으로 바꿔도 됨)
        m_enterAlpha = 1.0f;
        m_fadeAlpha = 1.0f;
        ApplyAlphaCombined();
    }

    void UIToastAnimator::Update()
    {
        if (!m_rt) return;

        const float dt = engine::Time::UnscaledDeltaTime();

        // 1) 이동(Enter/Move) 처리
        if (m_animating)
        {
            m_time += dt;
            float t = (m_duration > 0.0f) ? (m_time / m_duration) : 1.0f;
            t = Clamp01(t);

            m_rt->SetAnchoredPosition(Lerp(m_fromPos, m_toPos, t));

            if (m_entering)
                m_enterAlpha = 1.0f - (1.0f - t) * (1.0f - t);

            if (t >= 1.0f)
            {
                m_animating = false;
                m_rt->SetAnchoredPosition(m_toPos);

                if (m_entering) m_enterAlpha = 1.0f;

                m_entering = false;
            }
        }

        // 2) 페이드(FadeIn/FadeOut) 처리
        if (m_fading)
        {
            m_fadeTime += dt;

            const float dur = m_fadeModeIn ? m_fadeInDuration : m_fadeOutDuration;
            float t = (dur > 0.0f) ? (m_fadeTime / dur) : 1.0f;
            t = Clamp01(t);

            m_fadeAlpha = m_fadeModeIn ? t : (1.0f - t);

            if (t >= 1.0f)
            {
                m_fading = false;

                if (!m_fadeModeIn)
                    m_finished = true;
            }
        }

        // 3) 알파 적용은 여기서 딱 1번만
        ApplyAlphaCombined();
    }

    void UIToastAnimator::OnGui()
    {
        ImGui::DragFloat("Enter Slide Offset X", &m_enterSlideOffsetX, 1.0f, 0.0f, 2000.0f);
        ImGui::DragFloat("Enter Duration", &m_enterDuration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Move Duration", &m_moveDuration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("FadeIn Duration", &m_fadeInDuration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("FadeOut Duration", &m_fadeOutDuration, 0.01f, 0.01f, 2.0f);
    }

    void UIToastAnimator::Save(engine::json& j) const
    {
        Object::Save(j);
        j["EnterSlideOffsetX"] = m_enterSlideOffsetX;
        j["EnterDuration"] = m_enterDuration;
        j["MoveDuration"] = m_moveDuration;
        j["FadeInDuration"] = m_fadeInDuration;
        j["FadeOutDuration"] = m_fadeOutDuration;
    }

    void UIToastAnimator::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "EnterSlideOffsetX", m_enterSlideOffsetX);
        engine::JsonGet(j, "EnterDuration", m_enterDuration);
        engine::JsonGet(j, "MoveDuration", m_moveDuration);
        engine::JsonGet(j, "FadeInDuration", m_fadeInDuration);
        engine::JsonGet(j, "FadeOutDuration", m_fadeOutDuration);
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

        // Enter 시작
        m_finished = false;
        m_entering = true;

        m_toPos = targetPos;
        m_fromPos = { targetPos.x + m_enterSlideOffsetX, targetPos.y };

        m_duration = m_enterDuration;
        m_time = 0.0f;
        m_animating = true;

        // Enter 페이드인 켜져있으면 0에서 시작
        //m_enterAlpha = (m_fadeInOnEnter ? 0.0f : 1.0f);

        m_enterAlpha = 0.0f;
        m_fadeAlpha = 1.0f;

        m_rt->SetAnchoredPosition(m_fromPos);
        ApplyAlphaCombined();
    }

    void UIToastAnimator::MoveTo(const engine::Vector2& targetPos, float durationOverride)
    {
        if (!m_rt) return;

        m_finished = false;
        m_entering = false;

        m_fromPos = m_rt->GetAnchoredPosition();
        m_toPos = targetPos;

        m_duration = (durationOverride > 0.0f) ? durationOverride : m_moveDuration;
        m_time = 0.0f;
        m_animating = true;

        // Move는 알파에 영향 없음
        m_enterAlpha = 1.0f;
    }

    void UIToastAnimator::FadeIn(float durationOverride)
    {
        if (!m_inited)
        {
            CacheTargets();
            m_inited = true;
        }

        m_finished = false;
        m_fading = true;
        m_fadeModeIn = true;
        m_fadeTime = 0.0f;

        if (durationOverride > 0.0f)
            m_fadeInDuration = durationOverride;

        m_fadeAlpha = 0.0f;
        ApplyAlphaCombined();
    }

    void UIToastAnimator::FadeOut(float durationOverride)
    {
        if (!m_inited)
        {
            CacheTargets();
            m_inited = true;
        }

        m_finished = false;
        m_fading = true;
        m_fadeModeIn = false;
        m_fadeTime = 0.0f;

        if (durationOverride > 0.0f)
            m_fadeOutDuration = durationOverride;

        m_fadeAlpha = 1.0f;
        ApplyAlphaCombined();
    }

    void UIToastAnimator::SetText(const std::string& text)
    {
        auto* go = GetGameObject();
        auto* txt = go->GetComponent<engine::UIText>();
        if (!txt) return;

        txt->SetText(text);
    }

    // 최종 알파 = baseA * (enterAlpha * fadeAlpha)
    void UIToastAnimator::ApplyAlphaCombined()
    {
        const float a = Clamp01(m_enterAlpha) * Clamp01(m_fadeAlpha);

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

                // 원본 알파 캐시: 0이면 1로 보정 (재사용/초기 숨김에도 안전)
                at.baseA = text->GetColor().w;
                if (at.baseA <= 0.0001f) at.baseA = 1.0f;

                m_alphaTargets.push_back(at);
            }

            if (auto* img = go->GetComponent<engine::UIImage>())
            {
                AlphaTarget at;
                at.image = img;

                at.baseA = img->GetColor().w;
                if (at.baseA <= 0.0001f) at.baseA = 1.0f;

                m_alphaTargets.push_back(at);
            }
        }

        for (auto* c : t->GetChildren())
            CaptureBaseAlphaRecursive(c);
    }
}