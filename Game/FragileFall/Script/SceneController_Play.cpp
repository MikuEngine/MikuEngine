#include "GamePCH.h"
#include "SceneController_Play.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include "UIPopUpAnimator.h"

namespace game
{
    void SceneController_Play::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_OpenMenu", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenMenu(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToPlay", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToPlay(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToMain(); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->Back(); });
    }

    void SceneController_Play::Start()
    {
        m_menuPopUp = engine::GameObject::Find("Panel_Menu");
        if (m_menuPopUp) m_menuPopUp->SetActive(false);

        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_isMenuOpen = false;
        m_isOptionOpen = false;
    }

    void SceneController_Play::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            if (m_isOptionOpen) { Back(); return; }
            if (m_isMenuOpen) { SetMenuOpen(false);   return; }
            SetMenuOpen(true);
        }
    }

    void SceneController_Play::OnGui()
    {
    }

    void SceneController_Play::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SceneController_Play::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void SceneController_Play::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void SceneController_Play::SetMenuOpen(bool open)
    {
        m_isMenuOpen = open;

        if (m_menuPopUp)
        {
            if (auto* anim = m_menuPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();
            }
            else
            {
                m_menuPopUp->SetActive(open);
            }
        }

        UpdateBlocker();
    }

    void SceneController_Play::OpenMenu()
    {
        SetMenuOpen(true);
    }

    void SceneController_Play::OpenOption()
    {
        SetOptionOpen(true);
        SetMenuOpen(false);
    }

    void SceneController_Play::BackToPlay()
    {
        SetMenuOpen(false);
    }

    void SceneController_Play::BackToMain()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Title");
    }

    void SceneController_Play::Back()
    {
        SetOptionOpen(false);
        SetMenuOpen(true);
    }

    void SceneController_Play::UpdateBlocker()
    {
        if (!m_blocker) return;
        m_blocker->SetActive(m_isMenuOpen || m_isOptionOpen);
    }

    void SceneController_Play::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;
        if (m_optionPopUp)
        {
            if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();
            }
            else
            {
                m_optionPopUp->SetActive(open);
            }
        }

        UpdateBlocker();
    }
}