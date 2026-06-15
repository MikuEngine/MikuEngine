#include "GamePCH.h"
#include "UICurrentStageTitle.h"
#include <Framework/Object/Component/UI/UIText.h>
#include <Core/System/MyTime.h>

namespace game
{
    namespace
    {
        static float Clamp01(float v)
        {
            if (v < 0.0f) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        }

        static std::string BuildStageLabel(int stageNumber)
        {
            if (stageNumber == 10)
            {
                return "Boss";
            }

            return "Abyss Lv " + std::to_string(stageNumber);
        }
    }

    void UICurrentStageTitle::Awake()
    {

    }

    void UICurrentStageTitle::Start()
    {
        if (auto* go = engine::GameObject::Find("TitleText"))
            m_titleText = go->GetComponent<engine::UIText>();

        // 색깔 설정해두면 설정해둔 색깔로 반영
        m_color = m_titleText->GetColor();

        m_stageNumber = StageManager::Get().GetCurrentStage();

        m_elapsed = 0.0f;
        m_done = false;

        if (m_titleText) m_titleText->SetActive(true);

        if (m_titleText)
            m_titleText->SetText(BuildStageLabel(m_stageNumber));

        // 시작 알파 1
        if (m_titleText)
            m_titleText->SetColor({ m_color.x, m_color.y, m_color.z, 1.0f });
    }

    void UICurrentStageTitle::Update()
    {
        TimeCheck();
    }

    void UICurrentStageTitle::OnGui()
    {
        ImGui::DragFloat("FadeDelay", &m_fadeDelay, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("FadeDuration", &m_fadeDuration, 0.01f, 0.01f, 10.0f);
    }

    void UICurrentStageTitle::Save(engine::json& j) const
    {
        Object::Save(j);
        j["FadeDelay"] = m_fadeDelay;
        j["FadeDuration"] = m_fadeDuration;
    }

    void UICurrentStageTitle::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "FadeDelay", m_fadeDelay);
        engine::JsonGet(j, "FadeDuration", m_fadeDuration);
    }

    void UICurrentStageTitle::TimeCheck()
    {
        if (m_done) return;
        if (!m_titleText) return;

        const float dt = engine::Time::DeltaTime();
        m_elapsed += dt;

        if (m_elapsed < m_fadeDelay)
        {
            m_titleText->SetColor(engine::Vector4(m_color.x, m_color.y, m_color.z, 1.0f));
            return;
        }

        const float fadeElapsed = m_elapsed - m_fadeDelay;
        const float dur = (m_fadeDuration > 0.0001f) ? m_fadeDuration : 0.0001f;

        const float t = Clamp01(fadeElapsed / dur);
        const float a = 1.0f - t;

        m_titleText->SetColor(engine::Vector4(m_color.x, m_color.y, m_color.z, a));

        if (t >= 1.0f)
        {
            m_done = true;
            m_titleText->SetActive(false);
        }
    }
}