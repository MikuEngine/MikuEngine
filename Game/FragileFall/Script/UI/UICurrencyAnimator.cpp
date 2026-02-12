#include "GamePCH.h"
#include "UICurrencyAnimator.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Core/System/MyTime.h>

#include <Manager/StageManager.h>
#include <Core/System/Input.h>

namespace game
{
    namespace
    {
        static int StepTowards(int cur, int target, int stepAbs)
        {
            if (cur == target) return cur;
            if (stepAbs <= 0) return cur;

            const int diff = target - cur;
            const int ad = std::abs(diff);

            if (ad <= stepAbs) return target;
            return cur + (diff > 0 ? stepAbs : -stepAbs);
        }
    }

    void UICurrencyAnimator::Awake()
    {
        if (auto* go = engine::GameObject::Find("RubyCount"))
            m_rubyCount = go->GetComponent<engine::UIText>();

        if (auto* go = engine::GameObject::Find("SapphireCount"))
            m_sapphireCount = go->GetComponent<engine::UIText>();

        if (auto* go = engine::GameObject::Find("EmeraldCount"))
            m_emeraldCount = go->GetComponent<engine::UIText>();
    }

    void UICurrencyAnimator::Start()
    {
        if (!m_rubyCount || !m_sapphireCount || !m_emeraldCount) return;

        auto& sm = StageManager::Get();

        m_targetRuby = m_displayRuby = sm.GetRunRuby();
        m_targetSapphire = m_displaySapphire = sm.GetRunSapphire();
        m_targetEmerald = m_displayEmerald = sm.GetRunEmerald();

        ApplyText(m_rubyCount, m_displayRuby);
        ApplyText(m_sapphireCount, m_displaySapphire);
        ApplyText(m_emeraldCount, m_displayEmerald);

        m_initialized = true;
    }

    void UICurrencyAnimator::Update()
    {
        if (!m_initialized) return;
        if (!m_rubyCount || !m_sapphireCount || !m_emeraldCount) return;

        auto& sm = StageManager::Get();

        SetTargetValue(m_rubyCount, StageManager::Get().GetRunRuby());
        SetTargetValue(m_sapphireCount, StageManager::Get().GetRunSapphire());
        SetTargetValue(m_emeraldCount, StageManager::Get().GetRunEmerald());

        const float dt = engine::Time::DeltaTime();
        const float speed = std::abs(m_countSpeed);

        // Ruby
        {
            m_accRuby += speed * dt;
            const int step = (int)std::floor(m_accRuby);
            if (step > 0)
            {
                m_accRuby -= (float)step;

                const int prev = m_displayRuby;
                m_displayRuby = StepTowards(m_displayRuby, m_targetRuby, step);
                if (m_displayRuby != prev)
                    ApplyText(m_rubyCount, m_displayRuby);
            }
        }

        // Sapphire
        {
            m_accSapphire += speed * dt;
            const int step = (int)std::floor(m_accSapphire);
            if (step > 0)
            {
                m_accSapphire -= (float)step;

                const int prev = m_displaySapphire;
                m_displaySapphire = StepTowards(m_displaySapphire, m_targetSapphire, step);
                if (m_displaySapphire != prev)
                    ApplyText(m_sapphireCount, m_displaySapphire);
            }
        }

        // Emerald
        {
            m_accEmerald += speed * dt;
            const int step = (int)std::floor(m_accEmerald);
            if (step > 0)
            {
                m_accEmerald -= (float)step;

                const int prev = m_displayEmerald;
                m_displayEmerald = StepTowards(m_displayEmerald, m_targetEmerald, step);
                if (m_displayEmerald != prev)
                    ApplyText(m_emeraldCount, m_displayEmerald);
            }
        }
    }

    void UICurrencyAnimator::SetTargetValue(engine::UIText* text, int v)
    {
        if (!text) return;

        if (text == m_rubyCount)          m_targetRuby = v;
        else if (text == m_sapphireCount) m_targetSapphire = v;
        else if (text == m_emeraldCount)  m_targetEmerald = v;
    }

    void UICurrencyAnimator::AddDelta(engine::UIText* text, int delta)
    {
        if (!text) return;

        if (text == m_rubyCount)          m_targetRuby += delta;
        else if (text == m_sapphireCount) m_targetSapphire += delta;
        else if (text == m_emeraldCount)  m_targetEmerald += delta;
    }

    void UICurrencyAnimator::ApplyText(engine::UIText* text, int v)
    {
        if (!text) return;
        text->SetText(std::to_string(v));
    }

    void UICurrencyAnimator::TriggerPulse()
    {
    }

    void UICurrencyAnimator::TriggerColorFlash(int delta)
    {
    }

    void UICurrencyAnimator::UpdatePulse(float dt)
    {
    }

    void UICurrencyAnimator::UpdateColorFlash(float dt)
    {
    }

    engine::Vector4 UICurrencyAnimator::LerpColor(const engine::Vector4& a, const engine::Vector4& b, float t)
    {
        return engine::Vector4();
    }

    float UICurrencyAnimator::Clamp01(float v)
    {
        return 0.0f;
    }

    void UICurrencyAnimator::OnGui()
    {

    }

    void UICurrencyAnimator::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void UICurrencyAnimator::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}