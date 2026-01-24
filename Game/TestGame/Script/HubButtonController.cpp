#include "GamePCH.h"
#include "HubButtonController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/RectTransform.h>
#include "UIPopUpAnimator.h"

namespace game
{
    void HubButtonController::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_EnterPlay", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->EnterPlay(); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->OpenUpgrade(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->BackToMain(); });
        BindButton("UI_BackToHub", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->BackToHub(); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<HubButtonController>(this)]() {if (self) self->Back(); });

        BindButton("UI_EnterPlay", [self = engine::Ptr<HubButtonController>(this)](bool) {if (self) self->ShowEffect(); });
    }

    void HubButtonController::Start()
    {
        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_groupSelect = engine::GameObject::Find("UIGroup_Select");
        m_upgradePopUp = engine::GameObject::Find("UI_UpgradePopUp");

        m_isOptionOpen = false;
        m_isUpgradeOpen = false;

        SetOptionOpen(false);
        SetUpgradeOpen(false);
    }

    void HubButtonController::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
            HandleEscape();
    }

    void HubButtonController::OnGui()
    {

    }

    void HubButtonController::Save(engine::json& j) const
    {
        Object::Save(j);
        j["OptionOpen"] = m_isOptionOpen;
        j["UpgradeOpen"] = m_isUpgradeOpen;
    }

    void HubButtonController::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "OptionOpen", m_isOptionOpen);
        engine::JsonGet(j, "UpgradeOpen", m_isUpgradeOpen);
        SetOptionOpen(m_isOptionOpen);
        SetUpgradeOpen(m_isUpgradeOpen);
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
        SetUpgradeOpen(true);
    }

    void HubButtonController::OpenOption()
    {
        LOG_PRINT("OpenOption");
        SetOptionOpen(true);
    }

    void HubButtonController::BackToMain()
    {
        LOG_PRINT("BackToMain");
        engine::SceneManager::Get().ChangeScene("z_Hiro_Title");
    }

    void HubButtonController::BackToHub()
    {
        SetUpgradeOpen(false);
    }

    void HubButtonController::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;

        if (m_blocker)
            m_blocker->SetActive(open);

        if (!m_optionPopUp) return;
           
        if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
        {
            open ? anim->Open() : anim->Close();
        }
        else
        {
            m_optionPopUp->SetActive(open);
        }
    }

    void HubButtonController::SetUpgradeOpen(bool open)
    {
        m_isUpgradeOpen = open;

        if (m_blocker)
            m_blocker->SetActive(open);

        if (m_upgradePopUp)
            m_upgradePopUp->SetActive(open);

        if (!m_upgradePopUp) return;

        if (auto* anim = m_upgradePopUp->GetComponent<game::UIPopUpAnimator>())
        {
            open ? anim->Open() : anim->Close();
        }
        else
        {
            m_upgradePopUp->SetActive(open);
        }

        if (m_groupSelect)
            m_groupSelect->SetActive(!open);
    }

    void HubButtonController::HandleEscape()
    {
        if (m_isOptionOpen)
        {
            SetOptionOpen(false);
            return;
        }

        if (m_isUpgradeOpen)
        {
            SetUpgradeOpen(false);
            return;
        }
    }

    void HubButtonController::Back()
    {
        SetOptionOpen(false);
    }

    void HubButtonController::ShowEffect()
    {

    }
}