#include "GamePCH.h"
#include "SceneController_Play.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void SceneController_Play::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        m_menu = engine::GameObject::Find("Panel_Menu");
        m_blocker = engine::GameObject::Find("Panel_Blocker");

        BindButton("UI_OpenMenu", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenMenu(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToPlay", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToPlay(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToMain(); });
    }

    void SceneController_Play::Start()
    {
        m_menu = engine::GameObject::Find("Panel_Menu");
        m_blocker = engine::GameObject::Find("Panel_Blocker");
        SetMenuOpen(false);
    }

    void SceneController_Play::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            ToggleMenu();
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

    void SceneController_Play::ToggleMenu()
    {
        SetMenuOpen(!m_isMenuOpen);
    }

    void SceneController_Play::SetMenuOpen(bool open)
    {
        m_isMenuOpen = open;

        if (m_menu)    m_menu->SetActive(m_isMenuOpen);
        if (m_blocker) m_blocker->SetActive(m_isMenuOpen);
    }

    void SceneController_Play::OpenMenu()
    {
        SetMenuOpen(true);
    }

    void SceneController_Play::OpenOption()
    {
        LOG_PRINT("OpenOption");
    }

    void SceneController_Play::BackToPlay()
    {
        SetMenuOpen(false);
    }

    void SceneController_Play::BackToMain()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Title");
    }
}