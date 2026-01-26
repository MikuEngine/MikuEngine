#include "GamePCH.h"
#include "SceneController_Lobby.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/RectTransform.h>
#include "UIPopUpAnimator.h"

namespace game
{
    void SceneController_Lobby::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_EnterPlay", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->EnterPlay(); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->OpenUpgrade(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToMain(); });
        BindButton("UI_BackToHub", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToHub(); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->Back(); });

        BindButton("UI_EnterPlay", [self = engine::Ptr<SceneController_Lobby>(this)](bool) {if (self) self->ShowEffect(); });
    }

    void SceneController_Lobby::Start()
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

    void SceneController_Lobby::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
            HandleEscape();
    }

    void SceneController_Lobby::OnGui()
    {

    }

    void SceneController_Lobby::Save(engine::json& j) const
    {
        Object::Save(j);
        j["OptionOpen"] = m_isOptionOpen;
        j["UpgradeOpen"] = m_isUpgradeOpen;
    }

    void SceneController_Lobby::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "OptionOpen", m_isOptionOpen);
        engine::JsonGet(j, "UpgradeOpen", m_isUpgradeOpen);
        SetOptionOpen(m_isOptionOpen);
        SetUpgradeOpen(m_isUpgradeOpen);
    }

    void SceneController_Lobby::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void SceneController_Lobby::BindButton(const std::string& name, engine::UIButton::HoverCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnHover(std::move(cb));
    }

    void SceneController_Lobby::EnterPlay()
    {
        LOG_PRINT("EnterPlay");
        engine::SceneManager::Get().ChangeScene("z_Hiro_Play");
    }

    void SceneController_Lobby::OpenUpgrade()
    {
        SetUpgradeOpen(true);
    }

    void SceneController_Lobby::OpenOption()
    {
        LOG_PRINT("OpenOption");
        SetOptionOpen(true);
    }

    void SceneController_Lobby::BackToMain()
    {
        LOG_PRINT("BackToMain");
        engine::SceneManager::Get().ChangeScene("z_Hiro_Title");
    }

    void SceneController_Lobby::BackToHub()
    {
        SetUpgradeOpen(false);
    }

    void SceneController_Lobby::SetOptionOpen(bool open)
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

    void SceneController_Lobby::SetUpgradeOpen(bool open)
    {
        m_isUpgradeOpen = open;

        if (m_blocker)
            m_blocker->SetActive(open);

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

    void SceneController_Lobby::HandleEscape()
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

    void SceneController_Lobby::Back()
    {
        SetOptionOpen(false);
    }

    void SceneController_Lobby::ShowEffect()
    {

    }
}