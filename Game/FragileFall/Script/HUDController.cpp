#include "GamePCH.h"
#include "HUDController.h"
#include <algorithm>
#include <cmath>

#include <Manager/StageManager.h>

#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include <Framework/Object/Component/RectTransform.h>

#include "CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    void HUDController::Awake()
    {

    }

    void HUDController::Start()
    {
        CacheHearts();

        if (auto* go = engine::GameObject::Find("HitImage"))
            m_hitImage = go->GetComponent<engine::UIImage>();

        if (auto* go = engine::GameObject::Find("FragileImage"))
            m_fragileImage = go->GetComponent<engine::UIImage>();

        // Fragile Gauge (Canvas_HUD > Fragile Gauge > Fragile Gauge Progress)
        if (auto* go = engine::GameObject::Find("Fragile Gauge Progress"))
            m_fragileGaugeProgress = go->GetComponent<engine::UIProgressBar>();

        auto* go = engine::GameObject::Find("Player");
        if (!go) return;

        m_playerScript = go->GetComponent<PlayerControllerScript>();

        if (m_playerScript)
        {
            m_visibleHeartCount = CalcVisibleHeartCountFromPlayer();
            m_halfHp = CalcHalfHPFromPlayer();
            ApplyHearts();
            m_playerScript->SetOnDamaged([self = engine::Ptr<HUDController>(this)] {
                if (!self) return;

                if (self->m_hitImage)
                {
                    self->m_hitImage->SetEffect(engine::UIEffectType::ScreenHit);

                    self->m_hitImage->SetEffectParam(0, { 1.0f, 0.0f, 0.0f, 0.0f });
                }
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

        if (m_fragileGaugeProgress && m_playerScript)
        {
            float maxVal = m_playerScript->GetFragileGaugeMax();
            float currentVal = m_playerScript->GetFragileGaugeCurrent();
            float ratio = (maxVal > 0.0f) ? (currentVal / maxVal) : 0.0f;
            m_fragileGaugeProgress->SetValue(ratio);

            if (ratio > 0.0f)
            {
                m_fragileImage->SetActive(true);
                m_fragileImage->SetEffect(engine::UIEffectType::PurpleCorruption);

                // 이미지 알파는 항상 0 (셰이더에서 제어)
                auto col = m_fragileImage->GetColor();
                m_fragileImage->SetColor({ col.x, col.y, col.z, 0.0f });

                float intensity = 0.0f;

                // 60% 이상일 때: 강도 0.4 고정 (화면의 40% 정도 잠식)
                if (ratio >= 0.6f)
                {
                    intensity = 0.3f;
                }
                // 30% 이상일 때: 강도 0.1 ~ 0.4 사이를 부드럽게 (혹은 0.1 고정)
                else if (ratio >= 0.3f)
                {
                    intensity = 0.15f;
                }
                // 0% ~ 30% 미만: 강도 0.0 ~ 0.1 미만 부드럽게
                else
                {
                    float t = ratio / 0.3f;
                    intensity = t * 0.1f;
                }

                intensity = std::clamp(intensity, 0.0f, 0.4f);

                m_fragileImage->SetEffectParam(0, { intensity, 0.0f, 0.0f, 0.0f });
            }
            else if (m_fragileImage->IsActive())
            {
                m_fragileImage->ClearEffect();
                m_fragileImage->SetActive(false);
            }
        }

        if (m_playerScript)
        {
            int newVisibleHearts = CalcVisibleHeartCountFromPlayer();
            int newHalfHp = CalcHalfHPFromPlayer();
            if (newVisibleHearts != m_visibleHeartCount || newHalfHp != m_halfHp)
            {
                m_visibleHeartCount = newVisibleHearts;
                m_halfHp = newHalfHp;
                ApplyHearts();
            }
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

    void HUDController::CacheHearts()
    {
        if (m_cached) return;

        // Heart(0)~Heart(4) 캐싱
        for (int i = 0; i < kHeartCount; ++i)
        {
            const std::string name = "Heart(" + std::to_string(i) + ")";
            if (auto* go = engine::GameObject::Find(name))
            {
                m_heartGO[i] = go;
                m_hearts[i] = go->GetComponent<engine::UIImage>();
                m_heartRT[i] = go->GetComponent<engine::RectTransform>();

                // 마스크 기본 모드(전체 표시)
                if (m_hearts[i])
                    m_hearts[i]->SetMaskMode(engine::MaskMode::None);
            }
        }

        m_cached = true;
    }

    void HUDController::OnDamagedHalf()
    {
        m_halfHp -= 1;
        if (m_halfHp < 0) m_halfHp = 0;

        ApplyHearts();
    }

    int HUDController::CalcHalfHPFromPlayer() const
    {
        if (!m_playerScript) return 0;

        const float hpPerHalfHeart = CalcHpPerHalfHeart();
        const float currentHp = std::max(0.0f, m_playerScript->GetCurrentHp());
        int half = static_cast<int>(std::floor((currentHp + 0.0001f) / hpPerHalfHeart));

        const int maxHalfHp = m_visibleHeartCount * 2;
        if (half < 0) half = 0;
        if (half > maxHalfHp) half = maxHalfHp;

        return half;
    }

    int HUDController::CalcVisibleHeartCountFromPlayer() const
    {
        if (!m_playerScript) return kBaseHeartCount;

        const int hpPerHeart = CalcHpPerHeart();
        const float maxHp = m_playerScript->GetMaxHp();
        int hearts = static_cast<int>(std::round(maxHp / static_cast<float>(hpPerHeart)));
        hearts = std::clamp(hearts, 1, kHeartCount);
        return hearts;
    }

    int HUDController::CalcHpPerHeart() const
    {
        if (!m_playerScript)
            return 20;

        const float baseMaxHp = std::max(1.0f, m_playerScript->GetBaseMaxHp());
        return std::max(1, static_cast<int>(std::round(baseMaxHp / static_cast<float>(kBaseHeartCount))));
    }

    float HUDController::CalcHpPerHalfHeart() const
    {
        const float hpPerHeart = static_cast<float>(CalcHpPerHeart());
        return std::max(0.5f, hpPerHeart * 0.5f);
    }

    void HUDController::ApplyHearts()
    {
        for (int i = 0; i < kHeartCount; ++i)
        {
            if (m_heartGO[i])
            {
                m_heartGO[i]->SetActive(i < m_visibleHeartCount);
            }

            if (i >= m_visibleHeartCount)
                continue;

            const int filled = m_halfHp - i * 2; // 이 하트에 배정된 half(2/1/0)
            if (filled >= 2)      SetHeartFull(i);
            else if (filled == 1) SetHeartHalf(i);
            else                  SetHeartEmpty(i);
        }
    }

    engine::Vector4 HUDController::GetHeartFullClip(int i) const
    {
        if (!m_heartRT[i]) return { 0,0,0,0 };

        const auto r = m_heartRT[i]->GetWorldRect();
        return { r.x, r.y, r.x + r.w, r.y + r.h };
    }

    engine::Vector4 HUDController::GetHeartHalfClip(int i) const
    {
        if (!m_heartRT[i]) return { 0,0,0,0 };

        const auto r = m_heartRT[i]->GetWorldRect();
        return { r.x, r.y, r.x + r.w * 0.5f, r.y + r.h };
    }

    void HUDController::SetHeartFull(int i)
    {
        if (!m_hearts[i]) return;

        m_hearts[i]->SetColor({ 1,1,1,1 });
        m_hearts[i]->SetMaskMode(engine::MaskMode::None);

        m_hearts[i]->SetClipRect(GetHeartFullClip(i));
    }

    void HUDController::SetHeartHalf(int i)
    {
        if (!m_hearts[i]) return;

        m_hearts[i]->SetColor({ 1,1,1,1 });
        
        // Half일때 마스크 Rect모드로
        m_hearts[i]->SetMaskMode(engine::MaskMode::Rect);

        m_hearts[i]->SetClipRect(GetHeartHalfClip(i));
    }

    void HUDController::SetHeartEmpty(int i)
    {
        if (!m_hearts[i]) return;

        m_hearts[i]->SetColor({ 1,1,1,0 });
        m_hearts[i]->SetMaskMode(engine::MaskMode::None);
    }

}