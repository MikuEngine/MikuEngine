#include "GamePCH.h"
#include "SceneController_Main.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include "UIPopUpAnimator.h"

namespace game
{
    void SceneController_Main::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_StartButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->StartGame();});
        BindButton("UI_OptionButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->OpenOption();});
        BindButton("UI_CreditButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->OpenCredit();});
        BindButton("UI_QuitButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->QuitGame();});
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->Back();});
        BindButton("UI_CloseButton_Credit", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->Back();});
    }

    void SceneController_Main::Start()
    {
        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_creditPopUp = engine::GameObject::Find("UI_CreditPopUp");
        if (m_creditPopUp) m_creditPopUp->SetActive(false);
       
        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_isOptionOpen = false;
        m_isCreditOpen = false;
    }

    void SceneController_Main::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            if (m_isOptionOpen || m_isCreditOpen)
                Back();
        }
    }

    void SceneController_Main::OnGui()
    {

    }

    void SceneController_Main::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SceneController_Main::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void SceneController_Main::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void SceneController_Main::StartGame()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Hub");
    }

    void SceneController_Main::OpenOption()
    {
        LOG_PRINT("OpenOption");

        SetOptionOpen(true);
        SetCreditOpen(false);
    }

    void SceneController_Main::OpenCredit()
    {
        LOG_PRINT("OpenCredit");

        SetCreditOpen(true);
        SetOptionOpen(false);
    }

    void SceneController_Main::QuitGame()
    {
        LOG_PRINT("QuitGame");
    }

    void SceneController_Main::Back()
    {
        SetOptionOpen(false);
        SetCreditOpen(false);
    }

    void SceneController_Main::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;
        if (m_optionPopUp)
        {
            if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();
            }
        }

        UpdateBlocker();
    }

    void SceneController_Main::SetCreditOpen(bool open)
    {
        m_isCreditOpen = open;
        if (m_creditPopUp)
        {
            if (auto* anim = m_creditPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();
            }
        }

        UpdateBlocker();
    }

    void SceneController_Main::UpdateBlocker()
    {
        if (!m_blocker) return;
        m_blocker->SetActive(m_isOptionOpen || m_isCreditOpen);
    }
}