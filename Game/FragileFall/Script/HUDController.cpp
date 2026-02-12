#include "GamePCH.h"
#include "HUDController.h"
#include <algorithm>
#include <cmath>

#include <Manager/StageManager.h>

#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Canvas.h>

#include "CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    bool HUDController::s_hasHpSnapshot = false;
    float HUDController::s_cachedBaseMaxHp = 0.0f;
    float HUDController::s_cachedFinalMaxHp = 0.0f;
    float HUDController::s_cachedCurrentHp = 0.0f;

    void HUDController::UpdateHpSnapshot(float baseMaxHp, float finalMaxHp, float currentHp)
    {
        s_cachedBaseMaxHp = std::max(1.0f, baseMaxHp);
        s_cachedFinalMaxHp = std::max(1.0f, finalMaxHp);
        s_cachedCurrentHp = std::max(0.0f, currentHp);
        s_hasHpSnapshot = true;
    }

    void HUDController::ClearHpSnapshot()
    {
        s_cachedBaseMaxHp = 0.0f;
        s_cachedFinalMaxHp = 0.0f;
        s_cachedCurrentHp = 0.0f;
        s_hasHpSnapshot = false;
    }

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

        TryBindPlayer();
        m_visibleHalfSlots = CalcVisibleHalfSlotsFromPlayer();
        m_filledHalfSlots = CalcFilledHalfSlotsFromPlayer(m_visibleHalfSlots);
        ApplyHearts();
        ValidateHeartsOnStageEntry();
    }

    void HUDController::Update()
    {
        TryBindPlayer();

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

        if (m_playerScript && !m_damageCallbackBound)
        {
            m_playerScript->SetOnDamaged([self = engine::Ptr<HUDController>(this)] {
                if (!self) return;

                if (self->m_hitImage)
                {
                    self->m_hitImage->SetEffect(engine::UIEffectType::ScreenHit);
                    self->m_hitImage->SetEffectParam(0, { 1.0f, 0.0f, 0.0f, 0.0f });
                }
                });
            m_damageCallbackBound = true;
        }

        int newVisibleHalfSlots = CalcVisibleHalfSlotsFromPlayer();
        int newFilledHalfSlots = CalcFilledHalfSlotsFromPlayer(newVisibleHalfSlots);
        if (m_forceApplyHearts
            || newVisibleHalfSlots != m_visibleHalfSlots
            || newFilledHalfSlots != m_filledHalfSlots)
        {
            m_visibleHalfSlots = newVisibleHalfSlots;
            m_filledHalfSlots = newFilledHalfSlots;
            ApplyHearts();
            m_forceApplyHearts = false;
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
                if (i >= kBaseHeartCount)
                {
                    if (auto* tr = go->GetTransform())
                    {
                        if (auto* parent = tr->GetParent())
                        {
                            m_heartCaseGO[i] = parent->GetGameObject();
                        }
                    }
                }

                // 마스크 기본 모드(전체 표시)
                if (m_hearts[i])
                    m_hearts[i]->SetMaskMode(engine::MaskMode::None);
            }
        }

        m_cached = true;
    }

    void HUDController::TryBindPlayer()
    {
        PlayerControllerScript* nextPlayer = nullptr;
        if (auto* go = engine::GameObject::Find("Player"))
        {
            nextPlayer = go->GetComponent<PlayerControllerScript>();
        }

        if (nextPlayer != m_playerScript)
        {
            m_playerScript = nextPlayer;
            m_damageCallbackBound = false;
            m_forceApplyHearts = true;

            if (m_playerScript)
            {
                RefreshHpSnapshotFromPlayer();
                ValidateHeartsOnStageEntry();
            }
        }
    }

    void HUDController::RefreshHpSnapshotFromPlayer()
    {
        if (!m_playerScript)
            return;

        UpdateHpSnapshot(
            m_playerScript->GetBaseMaxHp(),
            m_playerScript->GetMaxHp(),
            m_playerScript->GetCurrentHp());
    }

    void HUDController::ValidateHeartsOnStageEntry()
    {
        const int expectedVisibleHalfSlots = CalcVisibleHalfSlotsFromPlayer();
        const int expectedFilledHalfSlots = CalcFilledHalfSlotsFromPlayer(expectedVisibleHalfSlots);
        if (expectedVisibleHalfSlots != m_visibleHalfSlots
            || expectedFilledHalfSlots != m_filledHalfSlots)
        {
            m_visibleHalfSlots = expectedVisibleHalfSlots;
            m_filledHalfSlots = expectedFilledHalfSlots;
            ApplyHearts();
        }
    }

    void HUDController::OnDamagedHalf()
    {
        m_filledHalfSlots -= 1;
        if (m_filledHalfSlots < 0) m_filledHalfSlots = 0;

        ApplyHearts();
    }

    int HUDController::CalcFilledHalfSlotsFromPlayer(int visibleHalfSlots) const
    {
        const float hpPerHalfHeart = CalcHpPerHalfHeart();
        float currentHp = 0.0f;
        if (m_playerScript)
        {
            currentHp = std::max(0.0f, m_playerScript->GetCurrentHp());
        }
        else
        {
            // 플레이어 바인딩 전 프레임은 스테이지 보존 HP를 사용해 하트 오표시를 방지한다.
            currentHp = std::max(0.0f, StageManager::Get().GetRunHP());
        }
        int filledHalfSlots = static_cast<int>(std::floor((currentHp / hpPerHalfHeart) + 0.0001f));
        filledHalfSlots = std::clamp(filledHalfSlots, 0, visibleHalfSlots);
        return filledHalfSlots;
    }

    int HUDController::CalcVisibleHalfSlotsFromPlayer() const
    {
        const float hpPerHalfHeart = CalcHpPerHalfHeart();
        float maxHp = static_cast<float>(kBaseHeartCount);
        if (m_playerScript)
        {
            maxHp = m_playerScript->GetMaxHp();
        }
        else if (s_hasHpSnapshot)
        {
            maxHp = s_cachedFinalMaxHp;
        }

        int maxHalfSlots = static_cast<int>(std::floor((maxHp / hpPerHalfHeart) + 0.0001f));
        maxHalfSlots = std::clamp(maxHalfSlots, 1, kHeartCount * 2);
        return maxHalfSlots;
    }

    float HUDController::CalcHpPerHalfHeart() const
    {
        // CurrentHP 표시는 반하트(0.5 HP) 단위로 고정한다.
        return 0.5f;
    }

    void HUDController::ApplyHearts()
    {
        const int visibleHeartCount = std::clamp((m_visibleHalfSlots + 1) / 2, 1, kHeartCount);
        for (int i = 0; i < kHeartCount; ++i)
        {
            if (m_heartGO[i])
            {
                m_heartGO[i]->SetActive(i < visibleHeartCount);
            }
            if (i >= kBaseHeartCount && m_heartCaseGO[i])
            {
                m_heartCaseGO[i]->SetActive(i < visibleHeartCount);
            }

            if (i >= visibleHeartCount)
                continue;

            const int filled = m_filledHalfSlots - i * 2; // 이 하트에 배정된 half(2/1/0)
            if (filled >= 2)      SetHeartFull(i);
            else if (filled >= 1) SetHeartHalf(i);
            else                  SetHeartEmpty(i);
        }
    }

    engine::Vector4 HUDController::GetHeartFullClip(int i) const
    {
        if (!m_hearts[i] || !m_heartRT[i]) return { 0,0,0,0 };

        engine::Canvas* canvas = m_hearts[i]->GetCanvasInParent();
        if (!canvas) return { 0,0,0,0 };

        const engine::Vector2 ref = canvas->GetReferenceResolution();
        engine::UIRect rootRect{ 0.0f, 0.0f, ref.x, ref.y };
        const auto r = m_heartRT[i]->GetWorldRectResolved(rootRect);
        return { r.x, r.y, r.x + r.w, r.y + r.h };
    }

    engine::Vector4 HUDController::GetHeartHalfClip(int i) const
    {
        if (!m_hearts[i] || !m_heartRT[i]) return { 0,0,0,0 };

        engine::Canvas* canvas = m_hearts[i]->GetCanvasInParent();
        if (!canvas) return { 0,0,0,0 };

        const engine::Vector2 ref = canvas->GetReferenceResolution();
        engine::UIRect rootRect{ 0.0f, 0.0f, ref.x, ref.y };
        const auto r = m_heartRT[i]->GetWorldRectResolved(rootRect);
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