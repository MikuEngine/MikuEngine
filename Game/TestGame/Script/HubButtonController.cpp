#include "GamePCH.h"
#include "HubButtonController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>

namespace game
{
    void HubButtonController::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        SetGuide("");

        BindButton("UI_EnterPlay", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->EnterPlay(); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->OpenUpgrade(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->BackToMain(); });
        BindButton("UI_BackToHub", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->BackToHub(); });

        BindButton("UI_EnterPlay", [self = engine::Ptr<HubButtonController>(this)](bool enter) {if (self && enter) self->SetGuide("EnterPlay"); else self->SetGuide(""); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<HubButtonController>(this)](bool enter) {if (self && enter) self->SetGuide("OpenUpgrade"); else self->SetGuide(""); });
        BindButton("UI_OpenOption", [self = engine::Ptr<HubButtonController>(this)](bool enter) {if (self && enter) self->SetGuide(""); else self->SetGuide(""); });
        BindButton("UI_BackToMain", [self = engine::Ptr<HubButtonController>(this)](bool enter) {if (self && enter) self->SetGuide(""); else self->SetGuide(""); });
    }

    void HubButtonController::OnGui()
    {

    }

    void HubButtonController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void HubButtonController::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void HubButtonController::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void HubButtonController::BindButton(const std::string& name, engine::UIButton::HoverCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnHover(std::move(cb));
    }

    void HubButtonController::EnterPlay()
    {
        LOG_PRINT("EnterPlay");
        engine::SceneManager::Get().ChangeScene("z_Hiro_Play");
    }

    void HubButtonController::OpenUpgrade()
    {
        auto* selectGO = engine::GameObject::Find("UIGroop_Select");
        if (!selectGO) return;

        selectGO->SetActive(false);

        auto* upgradeGO = engine::GameObject::Find("UIGroop_Upgrade");
        if (!upgradeGO) return;

        upgradeGO->SetActive(true);
    }

    void HubButtonController::OpenOption()
    {
        LOG_PRINT("OpenOption");
    }

    void HubButtonController::BackToMain()
    {
        LOG_PRINT("BackToMain");
        engine::SceneManager::Get().ChangeScene("z_Hiro_Title");
    }

    void HubButtonController::BackToHub()
    {
        auto* selectGO = engine::GameObject::Find("UIGroop_Select");
        if (!selectGO) return;

        selectGO->SetActive(true);

        auto* upgradeGO = engine::GameObject::Find("UIGroop_Upgrade");
        if (!upgradeGO) return;

        upgradeGO->SetActive(false);
    }

    void HubButtonController::SetGuide(const std::string& key)
    {
        auto* guideImage = engine::GameObject::Find("UI_GuideImage");
        if (!guideImage) return;
        auto* img = guideImage->GetComponent<engine::UIImage>();
        if (!img) return;

        auto* guideText = engine::GameObject::Find("UI_GuideText");
        if (!guideText) return;
        auto* txt = guideText->GetComponent<engine::UIText>();
        if (!txt) return;

        // key -> 텍스처 경로 매핑
        if (key == "EnterPlay")
        {
            img->SetTexture("Resource/Texture/UI/Image/guide_play.png");
            txt->SetText("어둠 속으로 모험을\n 떠나세요.");
        }
        else if(key == "OpenUpgrade")
        {
            img->SetTexture("Resource/Texture/UI/Image/guide_upgrade.png");
            txt->SetText("더욱 강력해진 모습으로\n 심연에 도전하세요.");
        }
        else if (key == "OpenOption") img->SetTexture("Resource/Texture/UI/Image/guide_option.png");
        else if (key == "BackToMain") img->SetTexture("Resource/Texture/UI/Image/guide_back.png");
        else { img->SetTexture("None"); txt->SetText(""); }
    }
}