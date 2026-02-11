#include "GamePCH.h"
#include "UICurrencyAnimator.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    namespace
    {
        static int StepTowards(int cur, int target, int stepAbs)
        {
            if (cur == target) return cur;
            if (stepAbs <= 0) return target;

            const int diff = target - cur;
            const int ad = std::abs(diff);

            if (ad <= stepAbs) return target;
            return cur + (diff > 0 ? stepAbs : -stepAbs);
        }
    }

    void UICurrencyAnimator::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_text = go->GetComponent<engine::UIText>();
    }

    void UICurrencyAnimator::Start()
    {
        m_displayValue = m_targetValue;
        ApplyText(m_displayValue);
        m_initialized = true;
    }

    void UICurrencyAnimator::Update()
    {
        if (!m_text) return;

        if (!m_initialized)
        {
            m_displayValue = m_targetValue;
            ApplyText(m_displayValue);
            m_initialized = true;
            return;
        }

        if (m_displayValue == m_targetValue) return;

        const float dt = engine::Time::DeltaTime();
        const int step = std::max(1, (int)std::floor(std::abs(m_countSpeed) * dt));

        m_displayValue = StepTowards(m_displayValue, m_targetValue, step);
        ApplyText(m_displayValue);
    }

    void UICurrencyAnimator::SetTargetValue(int v)
    {
        m_targetValue = v;

        if (m_text && !m_initialized)
        {
            m_displayValue = m_targetValue;
            ApplyText(m_displayValue);
            m_initialized = true;
        }
    }

    void UICurrencyAnimator::AddDelta(int delta)
    {
        SetTargetValue(m_targetValue + delta);
    }

    void UICurrencyAnimator::ApplyText(int v)
    {
        if (!m_text) return;

        m_text->SetText(std::to_string(v));
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