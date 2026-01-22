#include "GamePCH.h"
#include "UIInputHandler.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

namespace game
{
    void UIInputHandler::Awake()
    {
        m_menu = engine::SceneManager::Get().GetScene()->FindGameObject("Menu");
    }

    void UIInputHandler::Start()
    {
        m_isMenuOpen = false;
        m_menu->SetActive(m_isMenuOpen);
    }

    void UIInputHandler::Update()
    {
        if (!m_menu) return;

        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
            m_isMenuOpen = !m_isMenuOpen;

        m_menu->SetActive(m_isMenuOpen);
    }

    void UIInputHandler::OnGui()
    {
        
    }

    void UIInputHandler::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Menu"] = m_isMenuOpen;
    }

    void UIInputHandler::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Menu", m_isMenuOpen);
    }
}