#include "GamePCH.h"
#include "PlayButtonController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void PlayButtonController::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        m_menu = engine::GameObject::Find("Panel_Menu");
        m_blocker = engine::GameObject::Find("Panel_Blocker");

        BindButton("UI_OpenMenu", [self = engine::Ptr<PlayButtonController>(this)]() {if (self) self->OpenMenu(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<PlayButtonController>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToPlay", [self = engine::Ptr<PlayButtonController>(this)]() {if (self) self->BackToPlay(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<PlayButtonController>(this)]() {if (self) self->BackToMain(); });
    }

    void PlayButtonController::Start()
    {
        m_menu = engine::GameObject::Find("Panel_Menu");
        m_blocker = engine::GameObject::Find("Panel_Blocker");
        SetMenuOpen(false);
    }

    void PlayButtonController::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            ToggleMenu();
        }
    }

    void PlayButtonController::OnGui()
    {
    }

    void PlayButtonController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void PlayButtonController::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void PlayButtonController::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void PlayButtonController::ToggleMenu()
    {
        SetMenuOpen(!m_isMenuOpen);
    }

    void PlayButtonController::SetMenuOpen(bool open)
    {
        m_isMenuOpen = open;

        if (m_menu)    m_menu->SetActive(m_isMenuOpen);
        if (m_blocker) m_blocker->SetActive(m_isMenuOpen);
    }

    void PlayButtonController::OpenMenu()
    {
        SetMenuOpen(true);
    }

    void PlayButtonController::OpenOption()
    {
        LOG_PRINT("OpenOption");
    }

    void PlayButtonController::BackToPlay()
    {
        SetMenuOpen(false);
    }

    void PlayButtonController::BackToMain()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Title");
    }
}