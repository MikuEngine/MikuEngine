#include "GamePCH.h"
#include "UICurrencyAnimator.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Core/System/MyTime.h>

#include <Manager/StageManager.h>

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

        // 1) 타겟 갱신 (실제 값)
        m_targetRuby = sm.GetRunRuby();
        m_targetSapphire = sm.GetRunSapphire();
        m_targetEmerald = sm.GetRunEmerald();

        // 2) 이미 다 따라잡았으면 종료
        const bool done =
            (m_displayRuby == m_targetRuby) &&
            (m_displaySapphire == m_targetSapphire) &&
            (m_displayEmerald == m_targetEmerald);

        if (done) return;

        const float dt = engine::Time::DeltaTime();

        // 기존 주인님 로직 유지: dt 폭주 시 상한
        const int rawStep = std::max(1, (int)std::floor(std::abs(m_countSpeed) * dt));
        const int step = std::min(rawStep, 50);

        // 3) 각 재화 display -> target 스텝 이동 + ApplyText
        {
            const int prev = m_displayRuby;
            m_displayRuby = StepTowards(m_displayRuby, m_targetRuby, step);
            if (m_displayRuby != prev)
                ApplyText(m_rubyCount, m_displayRuby);
        }

        {
            const int prev = m_displaySapphire;
            m_displaySapphire = StepTowards(m_displaySapphire, m_targetSapphire, step);
            if (m_displaySapphire != prev)
                ApplyText(m_sapphireCount, m_displaySapphire);
        }

        {
            const int prev = m_displayEmerald;
            m_displayEmerald = StepTowards(m_displayEmerald, m_targetEmerald, step);
            if (m_displayEmerald != prev)
                ApplyText(m_emeraldCount, m_displayEmerald);
        }
    }

    void UICurrencyAnimator::SetTargetValue(int v)
    {
        m_targetValue = v;

        if (m_rubyCount && !m_initialized)
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

    void UICurrencyAnimator::ApplyText(engine::UIText* text, int v)
    {
        if (!text) return;
        text->SetText(std::to_string(v));
    }

    void UICurrencyAnimator::ApplyText(int v)
    {
        // 기존 코드 호환: 루비에만 쓰는 기본 동작
        ApplyText(m_rubyCount, v);
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
        j["countSpeed"] = m_countSpeed;
    }

    void UICurrencyAnimator::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "countSpeed", m_countSpeed);
    }
}