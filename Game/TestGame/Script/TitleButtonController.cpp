#include "GamePCH.h"
#include "TitleButtonController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void TitleButtonController::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_StartButton", [self = engine::Ptr<TitleButtonController>(this)]() {if (self) self->StartGame();});
        BindButton("UI_OptionButton", [self = engine::Ptr<TitleButtonController>(this)]() {if (self) self->OpenOption();});
        BindButton("UI_CreditButton", [self = engine::Ptr<TitleButtonController>(this)]() {if (self) self->OpenCredit();});
        BindButton("UI_QuitButton", [self = engine::Ptr<TitleButtonController>(this)]() {if (self) self->QuitGame();});
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<TitleButtonController>(this)]() {if (self) self->Back();});
        BindButton("UI_CloseButton_Credit", [self = engine::Ptr<TitleButtonController>(this)]() {if (self) self->Back();});
    }

    void TitleButtonController::Start()
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

    void TitleButtonController::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            if (m_isOptionOpen || m_isCreditOpen)
                Back();
        }
    }

    void TitleButtonController::OnGui()
    {

    }

    void TitleButtonController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TitleButtonController::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void TitleButtonController::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void TitleButtonController::StartGame()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Hub");
    }

    void TitleButtonController::OpenOption()
    {
        LOG_PRINT("OpenOption");

        SetOptionOpen(true);
        SetCreditOpen(false);
    }

    void TitleButtonController::OpenCredit()
    {
        LOG_PRINT("OpenCredit");

        SetCreditOpen(true);
        SetOptionOpen(false);
    }

    void TitleButtonController::QuitGame()
    {
        LOG_PRINT("QuitGame");
    }

    void TitleButtonController::Back()
    {
        SetOptionOpen(false);
        SetCreditOpen(false);
    }

    void TitleButtonController::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;
        if (m_optionPopUp) m_optionPopUp->SetActive(open);
        UpdateBlocker();
    }

    void TitleButtonController::SetCreditOpen(bool open)
    {
        m_isCreditOpen = open;
        if (m_creditPopUp) m_creditPopUp->SetActive(open);
        UpdateBlocker();
    }

    void TitleButtonController::UpdateBlocker()
    {
        if (!m_blocker) return;
        m_blocker->SetActive(m_isOptionOpen || m_isCreditOpen);
    }
}