#include "GamePCH.h"
#include "UIInputHandler.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

namespace game
{
    void UIInputHandler::Awake()
    {
        m_menuObj = engine::SceneManager::Get().GetScene()->FindGameObject("Menu");
    }

    void UIInputHandler::Start()
    {
        m_isMenuOpened = false;
        m_menuObj->SetActive(m_isMenuOpened);
    }

    void UIInputHandler::Update()
    {
        if (!m_menuObj) return;

        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
            m_isMenuOpened = !m_isMenuOpened;

        m_menuObj->SetActive(m_isMenuOpened);
    }

    void UIInputHandler::OnGui()
    {
        
    }

    void UIInputHandler::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Menu"] = m_isMenuOpened;
    }

    void UIInputHandler::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Menu", m_isMenuOpened);
    }

    std::string UIInputHandler::GetType() const
    {
        return "UIInputHandler";
    }
}