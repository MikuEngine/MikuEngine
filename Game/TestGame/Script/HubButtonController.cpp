#include "GamePCH.h"
#include "HubButtonController.h"

#include <Framework/Scene/SceneManager.h>

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
}