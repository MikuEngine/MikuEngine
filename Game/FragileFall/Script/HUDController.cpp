#include "GamePCH.h"
#include "HUDController.h"

#include <Manager/StageManager.h>

#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>

#include "CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    void HUDController::Awake()
    {

    }

    void HUDController::Start()
    {
        if (auto* go = engine::GameObject::Find("HitImage"))
            m_hitImage = go->GetComponent<engine::UIImage>();

        if (auto* go = engine::GameObject::Find("FragileImage"))
            m_fragileImage = go->GetComponent<engine::UIImage>();

        // HUD: Currency counts (Canvas_HUD > Currency > Ruby/Sapphire/Emerald > * Count)
        if (auto* go = engine::GameObject::Find("Ruby Count"))
            m_currencyRubyText = go->GetComponent<engine::UIText>();
        if (auto* go = engine::GameObject::Find("Sapphire Count"))
            m_currencySapphireText = go->GetComponent<engine::UIText>();
        if (auto* go = engine::GameObject::Find("Emerald Count"))
            m_currencyEmeraldText = go->GetComponent<engine::UIText>();
        // Fragile Gauge (Canvas_HUD > Fragile Gauge > Fragile Gauge Progress)
        if (auto* go = engine::GameObject::Find("Fragile Gauge Progress"))
            m_fragileGaugeProgress = go->GetComponent<engine::UIProgressBar>();

        auto* go = engine::GameObject::Find("Player");
        if (!go) return;

        m_playerScript = go->GetComponent<PlayerControllerScript>();

        if (m_playerScript)
        {
            m_playerScript->SetOnDamaged([this] {
                if (m_hitImage)
                {
                    m_hitImage->SetEffect(engine::UIEffectType::ScreenHit);

                    m_hitImage->SetEffectParam(0, { 1.0f, 0.0f, 0.0f, 0.0f });
                }

                // TODO: 사운드 있다면 여기에 추가
                });
        }
    }

    void HUDController::Update()
    {
        if (m_hitImage)
        {
            auto params = m_hitImage->GetEffectParam(0);

            // 강도가 남아있다면 매 프레임 감소
            if (params.x > 0.0f)
            {
                params.x -= engine::Time::DeltaTime() * 2.5f; // 약 0.4초 동안 페이드 아웃
                if (params.x < 0.0f) params.x = 0.0f;

                m_hitImage->SetEffectParam(0, params);
            }
        }

        // HUD: 런 재화 개수, 프레자일 게이지
        if (m_currencyRubyText)
            m_currencyRubyText->SetText(std::to_string(StageManager::Get().GetRunRuby()));
        if (m_currencySapphireText)
            m_currencySapphireText->SetText(std::to_string(StageManager::Get().GetRunSapphire()));
        if (m_currencyEmeraldText)
            m_currencyEmeraldText->SetText(std::to_string(StageManager::Get().GetRunEmerald()));
        if (m_fragileGaugeProgress && m_playerScript)
        {
            float maxVal = m_playerScript->GetFragileGaugeMax();
            float currentVal = m_playerScript->GetFragileGaugeCurrent();
            float ratio = (maxVal > 0.0f) ? (currentVal / maxVal) : 0.0f;
            m_fragileGaugeProgress->SetValue(ratio);

            // 프레자일 게이지 화면 연출 (보류)
            //if (ratio > 0.3f)
            //{
            //    m_fragileImage->SetActive(true);
            //    m_fragileImage->SetEffect(engine::UIEffectType::AbyssalDecay);

            //    // --- 단계별 강도 보정 (0.3, 0.6, 0.9에서 확 바뀌게) ---
            //    float intensity = 0.0f;

            //    if (ratio >= 0.9f) {
            //        // 90% 이상: 거의 화면을 다 덮는 수준 (강도 0.8 ~ 1.0)
            //        intensity = 0.8f + (ratio - 0.9f) * 2.0f;
            //    }ㅂㄷ
            //    else if (ratio >= 0.6f) {
            //        // 60% 이상: 시야를 절반 정도 가림 (강도 0.5 ~ 0.7)
            //        intensity = 0.5f + (ratio - 0.6f) * 0.66f;
            //    }
            //    else {
            //        // 30% 이상: 외곽 위주로 잠식 (강도 0.2 ~ 0.4)
            //        intensity = 0.2f + (ratio - 0.3f) * 0.66f;
            //    }

            //    // 값 범위 고정 (0.01 ~ 1.0) 
            //    intensity = std::max(0.01f, std::min(1.0f, intensity));

            //    // 셰이더의 intensity = saturate((g_time - startTime) / 1.0f) 이므로
            //    // startTime = g_time - intensity 가 되면 원하는 강도 지점으로 고정됨
            //    float fakeStartTime = engine::Time::UnscaledTime() - intensity;

            //    m_fragileImage->SetEffectParam(1, { fakeStartTime, 0.0f, 0.0f, 0.0f });
            //}
            //else
            //{
            //    // 30% 미만일 때 파라미터 초기화 후 비활성화
            //    m_fragileImage->SetEffectParam(1, { 0.0f, 0.0f, 0.0f, 0.0f });
            //    m_fragileImage->ClearEffect();
            //    m_fragileImage->SetActive(false);
            //}
        }
    }

    void HUDController::OnGui()
    {

    }

    void HUDController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void HUDController::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}