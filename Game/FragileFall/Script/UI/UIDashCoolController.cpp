#include "GamePCH.h"
#include "UIDashCoolController.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include <Framework/Object/Component/UI/UIImage.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    namespace
    {
        static engine::GameObject* FindChildByName(engine::GameObject* parent, const std::string& childName)
        {
            if (!parent || !parent->GetTransform())
                return nullptr;

            const auto& children = parent->GetTransform()->GetChildren();
            for (auto* childTransform : children)
            {
                if (!childTransform)
                    continue;

                engine::GameObject* child = childTransform->GetGameObject();
                if (!child)
                    continue;

                if (child->GetName() == childName)
                    return child;
            }
            return nullptr;
        }

        static void CollectUIImagesRecursive(engine::GameObject* root, std::vector<engine::UIImage*>& outImages)
        {
            if (!root || !root->GetTransform())
                return;

            if (auto* img = root->GetComponent<engine::UIImage>())
            {
                outImages.push_back(img);
            }

            const auto& children = root->GetTransform()->GetChildren();
            for (auto* childTransform : children)
            {
                if (!childTransform)
                    continue;
                CollectUIImagesRecursive(childTransform->GetGameObject(), outImages);
            }
        }

        static engine::Vector4 LerpColor(const engine::Vector4& a, const engine::Vector4& b, float t)
        {
            return engine::Vector4(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            );
        }
    }

    void UIDashCoolController::Awake()
    {
    }

    void UIDashCoolController::Start()
    {
        ResolveReferences();
        RefreshUIImageCacheIfNeeded();
        UpdateDashUI();
    }

    void UIDashCoolController::Update()
    {
        ResolveReferences();
        RefreshUIImageCacheIfNeeded();
        UpdateDashUI();
    }

    void UIDashCoolController::ResolveReferences()
    {
        if (!m_playerScript)
        {
            if (auto* playerGO = engine::GameObject::Find(m_playerObjectName))
            {
                m_playerScript = playerGO->GetComponent<PlayerControllerScript>();
            }
        }

        if (!m_dashCoolObject)
        {
            engine::GameObject* self = GetGameObject();
            if (self && self->GetName() == m_dashCoolObjectName)
            {
                m_dashCoolObject = self;
            }
            else if (self)
            {
                m_dashCoolObject = FindChildByName(self, m_dashCoolObjectName);
            }

            if (!m_dashCoolObject)
            {
                m_dashCoolObject = engine::GameObject::Find(m_dashCoolObjectName);
            }
        }

        if (!m_dashCoolObject)
            return;

        if (!m_fillProgressBar)
        {
            m_fillProgressBar = m_dashCoolObject->GetComponent<engine::UIProgressBar>();
        }

        if (!m_fillObject)
        {
            m_fillObject = FindChildByName(m_dashCoolObject.Get(), m_fillObjectName);
        }
        if (!m_fillImage && m_fillObject)
        {
            m_fillImage = m_fillObject->GetComponent<engine::UIImage>();
            if (m_fillImage)
            {
                m_fillDefaultColor = m_fillImage->GetColor();
                m_fillVisualColor = m_fillDefaultColor;
            }
        }
        if (!m_fillBackgroundImage)
        {
            if (auto* bgObject = FindChildByName(m_dashCoolObject.Get(), "Background"))
            {
                m_fillBackgroundImage = bgObject->GetComponent<engine::UIImage>();
                if (m_fillBackgroundImage)
                {
                    m_progressBackgroundBaseColor = m_fillBackgroundImage->GetColor();
                }
            }
        }

        if (!m_bgCaseObject)
        {
            m_bgCaseObject = FindChildByName(m_dashCoolObject.Get(), m_bgCaseName);
        }
        if (!m_bgDashBtnObject)
        {
            m_bgDashBtnObject = FindChildByName(m_dashCoolObject.Get(), m_bgDashBtnName);
        }

        if (!m_ringGauge1)
        {
            m_ringGauge1 = FindChildByName(m_dashCoolObject.Get(), m_ringGauge1Name);
        }
        if (!m_ringGauge2)
        {
            m_ringGauge2 = FindChildByName(m_dashCoolObject.Get(), m_ringGauge2Name);
        }
        if (!m_ringGauge3)
        {
            m_ringGauge3 = FindChildByName(m_dashCoolObject.Get(), m_ringGauge3Name);
        }
    }

    void UIDashCoolController::UpdateDashUI()
    {
        if (!m_playerScript || !m_fillProgressBar)
        {
            engine::GameObject* uiRoot = m_dashCoolObject ? m_dashCoolObject.Get() : GetGameObject();
            if (uiRoot && uiRoot->IsActive())
            {
                uiRoot->SetActive(false);
            }
            return;
        }

        engine::GameObject* uiRoot = m_dashCoolObject ? m_dashCoolObject.Get() : GetGameObject();
        if (uiRoot && !uiRoot->IsActive())
        {
            uiRoot->SetActive(true);
        }

        const float deltaTime = engine::Time::DeltaTime();
        const int currentDashCount = std::clamp(m_playerScript->GetCurrentDashCount(), 0, kDashSlotCount);
        const bool isFullDash = (currentDashCount >= kDashSlotCount);

        // Fill 충전률
        float fillValue = 1.0f;
        if (!isFullDash)
        {
            const float rechargeTime = std::max(0.0001f, m_playerScript->GetDashRechargeTime());
            const float rechargeTimer = std::clamp(m_playerScript->GetDashRechargeTimer(), 0.0f, rechargeTime);
            fillValue = 1.0f - (rechargeTimer / rechargeTime);
        }

        // 충전 완성 시점(대쉬 카운트 증가) 감지 후 Fill 깜빡임 시작
        if (m_prevDashCount >= 0 && currentDashCount > m_prevDashCount)
        {
            if (m_enableFillFlash)
            {
                // 충전 1칸 완성 연출 중에는 항상 가득 찬 상태를 유지한다.
                m_heldFillValue = 1.0f;
                m_holdFillValueDuringFlash = true;
                TriggerFillFlashByRecoveredDashCount(std::clamp(currentDashCount, 1, 3));
            }
        }
        m_prevDashCount = currentDashCount;

        // 플래시 중에는 Fill 진행값을 고정한다.
        if (m_fillFlashPlaying && m_holdFillValueDuringFlash)
        {
            m_fillProgressBar->SetValue(m_heldFillValue);
        }
        else
        {
            m_fillProgressBar->SetValue(std::clamp(fillValue, 0.0f, 1.0f));
        }

        // 링 게이지 활성화
        if (m_ringGauge1) m_ringGauge1->SetActive(currentDashCount == 1);
        if (m_ringGauge2) m_ringGauge2->SetActive(currentDashCount == 2);
        if (m_ringGauge3) m_ringGauge3->SetActive(currentDashCount == 3);

        const bool isDead = IsPlayerDead();
        if (isDead)
        {
            if (!m_playerDeathFadeStarted)
            {
                m_playerDeathFadeStarted = true;
            }
            m_targetAlpha = 0.0f;
        }
        else
        {
            m_playerDeathFadeStarted = false;

            // 풀충전 상태에서 일정 시간 후 전체 페이드아웃, 풀충전 해제 시 페이드인
            if (isFullDash)
            {
                m_fullDashElapsed += deltaTime;
                if (m_fullDashElapsed >= std::max(0.0f, m_fullDashFadeDelay))
                {
                    m_targetAlpha = 0.0f;
                }
                else
                {
                    m_targetAlpha = 1.0f;
                }
            }
            else
            {
                m_fullDashElapsed = 0.0f;
                m_targetAlpha = 1.0f;
            }
        }

        float fadeOutDuration = m_playerDeathFadeStarted ? m_playerDeathFadeOutDuration : m_fadeOutDuration;
        const float fadeDuration = (m_targetAlpha < m_currentAlpha)
            ? std::max(0.0001f, fadeOutDuration)
            : std::max(0.0001f, m_fadeInDuration);
        const float fadeStep = deltaTime / fadeDuration;
        if (m_targetAlpha > m_currentAlpha)
        {
            m_currentAlpha = std::min(m_targetAlpha, m_currentAlpha + fadeStep);
        }
        else
        {
            m_currentAlpha = std::max(m_targetAlpha, m_currentAlpha - fadeStep);
        }

        UpdateFillFlash(deltaTime);
        ApplyProgressBarColors();
        ApplyGlobalAlphaToUIImages(m_currentAlpha);
        ApplyUniformScaleToTargets();
    }

    void UIDashCoolController::RefreshUIImageCacheIfNeeded()
    {
        engine::GameObject* root = GetGameObject();
        if (!root)
            return;

        if (m_lastUIImageCacheRoot == root && !m_uiImages.empty())
            return;

        std::vector<engine::UIImage*> found;
        CollectUIImagesRecursive(root, found);

        m_uiImages.clear();
        m_uiImageBaseAlphas.clear();
        m_uiImages.reserve(found.size());
        m_uiImageBaseAlphas.reserve(found.size());

        for (auto* img : found)
        {
            if (!img)
                continue;
            if (img == m_fillImage.Get() || img == m_fillBackgroundImage.Get())
                continue;
            m_uiImages.push_back(img);
            m_uiImageBaseAlphas.push_back(img->GetColor().w);
        }

        m_lastUIImageCacheRoot = root;
    }

    void UIDashCoolController::ApplyGlobalAlphaToUIImages(float alpha01)
    {
        const float clampedAlpha = std::clamp(alpha01, 0.0f, 1.0f);
        const size_t count = std::min(m_uiImages.size(), m_uiImageBaseAlphas.size());
        for (size_t i = 0; i < count; ++i)
        {
            auto* img = m_uiImages[i].Get();
            if (!img)
                continue;

            engine::Vector4 color = img->GetColor();
            color.w = std::clamp(m_uiImageBaseAlphas[i] * clampedAlpha, 0.0f, 1.0f);
            img->SetColor(color);
        }
    }

    void UIDashCoolController::ApplyUniformScaleToTargets()
    {
        const float scale = std::max(0.01f, m_childUniformScale);
        auto apply = [scale, this](const engine::Ptr<engine::GameObject>& target, engine::Vector3& outBaseScale)
            {
                if (!target || !target->GetTransform())
                    return;

                if (!m_scaleBasesCached)
                {
                    outBaseScale = target->GetTransform()->GetLocalScale();
                }
                target->GetTransform()->SetLocalScale(outBaseScale * scale);
            };

        apply(m_bgCaseObject, m_bgCaseBaseScale);
        apply(m_bgDashBtnObject, m_bgDashBtnBaseScale);
        apply(m_ringGauge1, m_ringGauge1BaseScale);
        apply(m_ringGauge2, m_ringGauge2BaseScale);
        apply(m_ringGauge3, m_ringGauge3BaseScale);

        m_scaleBasesCached = true;
    }

    void UIDashCoolController::TriggerFillFlashByRecoveredDashCount(int recoveredDashCount)
    {
        if (!m_fillProgressBar)
            return;

        switch (recoveredDashCount)
        {
        case 1: m_fillFlashTargetColor = m_fillFlashColorDash1; break;
        case 2: m_fillFlashTargetColor = m_fillFlashColorDash2; break;
        case 3: m_fillFlashTargetColor = m_fillFlashColorDash3; break;
        default: return;
        }

        m_fillFlashElapsed = 0.0f;
        m_fillFlashPlaying = true;
    }

    void UIDashCoolController::UpdateFillFlash(float deltaTime)
    {
        if (!m_fillProgressBar)
            return;

        if (!m_fillFlashPlaying)
        {
            m_fillVisualColor = m_fillDefaultColor;
            return;
        }

        const float totalDuration = std::max(0.0001f, m_fillFlashDuration);
        m_fillFlashElapsed += std::max(0.0f, deltaTime);
        const float t = std::clamp(m_fillFlashElapsed / totalDuration, 0.0f, 1.0f);

        // 타임라인 비율: 0~2 목표색상으로 선형보간, 2~8 목표색상 유지, 8~10 기본색으로 복귀
        const float p1 = 0.2f;
        const float p2 = 0.8f;
        engine::Vector4 c = m_fillDefaultColor;
        if (t < p1)
        {
            const float localT = t / p1;
            c = LerpColor(m_fillDefaultColor, m_fillFlashTargetColor, localT);
        }
        else if (t < p2)
        {
            c = m_fillFlashTargetColor;
        }
        else
        {
            const float localT = (t - p2) / (1.0f - p2);
            c = LerpColor(m_fillFlashTargetColor, m_fillDefaultColor, localT);
        }

        m_fillVisualColor = c;

        if (m_fillFlashElapsed >= totalDuration)
        {
            m_fillVisualColor = m_fillDefaultColor;
            m_fillFlashPlaying = false;
            m_holdFillValueDuringFlash = false;
        }
    }

    void UIDashCoolController::ApplyProgressBarColors()
    {
        if (!m_fillProgressBar)
            return;

        engine::Vector4 bg = m_progressBackgroundBaseColor;
        bg.w = std::clamp(m_progressBackgroundBaseColor.w * m_currentAlpha, 0.0f, 1.0f);

        engine::Vector4 fill = m_fillVisualColor;
        fill.w = std::clamp(fill.w * m_currentAlpha, 0.0f, 1.0f);

        m_fillProgressBar->SetColors(bg, fill);
    }

    bool UIDashCoolController::IsPlayerDead() const
    {
        if (!m_playerScript)
            return false;

        if (m_playerScript->GetCurrentHp() <= 0.0f)
            return true;

        return (m_playerScript->GetCurrentState() == "Dead");
    }

    void UIDashCoolController::OnGui()
    {
        ImGui::DragFloat("Child Uniform Scale", &m_childUniformScale, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Full Dash Fade Delay", &m_fullDashFadeDelay, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Fade Out Duration", &m_fadeOutDuration, 0.05f, 0.0f, 10.0f);
        ImGui::DragFloat("Fade In Duration", &m_fadeInDuration, 0.05f, 0.0f, 10.0f);
        ImGui::DragFloat("Player Death Fade Out Duration", &m_playerDeathFadeOutDuration, 0.05f, 0.0f, 10.0f);

        ImGui::Separator();
        ImGui::Checkbox("Enable Fill Flash", &m_enableFillFlash);
        ImGui::DragFloat("Fill Flash Duration", &m_fillFlashDuration, 0.01f, 0.01f, 2.0f);
        ImGui::ColorEdit3("Fill Flash Color Dash1", &m_fillFlashColorDash1.x);
        ImGui::ColorEdit3("Fill Flash Color Dash2", &m_fillFlashColorDash2.x);
        ImGui::ColorEdit3("Fill Flash Color Dash3", &m_fillFlashColorDash3.x);

        ImGui::Separator();
        ImGui::InputText("Player Object Name", &m_playerObjectName);
        ImGui::InputText("DashCool Object Name", &m_dashCoolObjectName);
        ImGui::InputText("Fill Object Name", &m_fillObjectName);
        ImGui::InputText("BG_Case Name", &m_bgCaseName);
        ImGui::InputText("BG_Dash_Btn Name", &m_bgDashBtnName);
        ImGui::InputText("RingGauge_1 Name", &m_ringGauge1Name);
        ImGui::InputText("RingGauge_2 Name", &m_ringGauge2Name);
        ImGui::InputText("RingGauge_3 Name", &m_ringGauge3Name);

        if (m_playerScript)
        {
            ImGui::Text("Dash Count: %d / %d",
                m_playerScript->GetCurrentDashCount(),
                m_playerScript->GetMaxDashCount());
            ImGui::Text("Dash Recharge: %.2f / %.2f",
                m_playerScript->GetDashRechargeTimer(),
                m_playerScript->GetDashRechargeTime());
            ImGui::Text("UI Alpha: %.2f", m_currentAlpha);
        }
    }

    void UIDashCoolController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["ChildUniformScale"] = m_childUniformScale;
        j["FullDashFadeDelay"] = m_fullDashFadeDelay;
        j["FadeOutDuration"] = m_fadeOutDuration;
        j["FadeInDuration"] = m_fadeInDuration;
        j["PlayerDeathFadeOutDuration"] = m_playerDeathFadeOutDuration;
        j["EnableFillFlash"] = m_enableFillFlash;
        j["FillFlashDuration"] = m_fillFlashDuration;
        j["FillFlashColorDash1"] = m_fillFlashColorDash1;
        j["FillFlashColorDash2"] = m_fillFlashColorDash2;
        j["FillFlashColorDash3"] = m_fillFlashColorDash3;
        j["PlayerObjectName"] = m_playerObjectName;
        j["DashCoolObjectName"] = m_dashCoolObjectName;
        j["FillObjectName"] = m_fillObjectName;
        j["BGCaseName"] = m_bgCaseName;
        j["BGDashBtnName"] = m_bgDashBtnName;
        j["RingGauge1Name"] = m_ringGauge1Name;
        j["RingGauge2Name"] = m_ringGauge2Name;
        j["RingGauge3Name"] = m_ringGauge3Name;
    }

    void UIDashCoolController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "ChildUniformScale", m_childUniformScale);
        engine::JsonGet(j, "FullDashFadeDelay", m_fullDashFadeDelay);
        engine::JsonGet(j, "FadeOutDuration", m_fadeOutDuration);
        engine::JsonGet(j, "FadeInDuration", m_fadeInDuration);
        engine::JsonGet(j, "PlayerDeathFadeOutDuration", m_playerDeathFadeOutDuration);
        engine::JsonGet(j, "EnableFillFlash", m_enableFillFlash);
        engine::JsonGet(j, "FillFlashDuration", m_fillFlashDuration);
        engine::JsonGet(j, "FillFlashColorDash1", m_fillFlashColorDash1);
        engine::JsonGet(j, "FillFlashColorDash2", m_fillFlashColorDash2);
        engine::JsonGet(j, "FillFlashColorDash3", m_fillFlashColorDash3);
        engine::JsonGet(j, "PlayerObjectName", m_playerObjectName);
        engine::JsonGet(j, "DashCoolObjectName", m_dashCoolObjectName);
        engine::JsonGet(j, "FillObjectName", m_fillObjectName);
        engine::JsonGet(j, "BGCaseName", m_bgCaseName);
        engine::JsonGet(j, "BGDashBtnName", m_bgDashBtnName);
        engine::JsonGet(j, "RingGauge1Name", m_ringGauge1Name);
        engine::JsonGet(j, "RingGauge2Name", m_ringGauge2Name);
        engine::JsonGet(j, "RingGauge3Name", m_ringGauge3Name);

        m_playerScript = nullptr;
        m_dashCoolObject = nullptr;
        m_fillProgressBar = nullptr;
        m_fillObject = nullptr;
        m_fillImage = nullptr;
        m_fillBackgroundImage = nullptr;
        m_bgCaseObject = nullptr;
        m_bgDashBtnObject = nullptr;
        m_ringGauge1 = nullptr;
        m_ringGauge2 = nullptr;
        m_ringGauge3 = nullptr;
        m_uiImages.clear();
        m_uiImageBaseAlphas.clear();
        m_lastUIImageCacheRoot = nullptr;
        m_scaleBasesCached = false;
        m_prevDashCount = -1;
        m_fullDashElapsed = 0.0f;
        m_targetAlpha = 1.0f;
        m_currentAlpha = 1.0f;
        m_fillFlashPlaying = false;
        m_fillFlashElapsed = 0.0f;
        m_fillVisualColor = m_fillDefaultColor;
        m_playerDeathFadeStarted = false;
        m_holdFillValueDuringFlash = false;
        m_heldFillValue = 1.0f;
    }
}
