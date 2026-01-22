#include "GamePCH.h"
#include "MainMenuController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void MainMenuController::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_StartButton", [self = engine::Ptr<MainMenuController>(this)]() {if (self) self->OnStart();});
        BindButton("UI_OptionButton", [self = engine::Ptr<MainMenuController>(this)]() {if (self) self->OnOption();});
        BindButton("UI_CreditButton", [self = engine::Ptr<MainMenuController>(this)]() {if (self) self->OnCredit();});
        BindButton("UI_QuitButton", [self = engine::Ptr<MainMenuController>(this)]() {if (self) self->OnQuit();});
    }

    void MainMenuController::Start()
    {
    }
    void MainMenuController::Update()
    {
    }

    void MainMenuController::OnGui()
    {
        if (ImGui::Button("Start"))
            OnStart();

        if (ImGui::Button("Option"))
            OnOption();

        if (ImGui::Button("Credit"))
            OnCredit();

        if (ImGui::Button("Quit"))
            OnQuit();
    }

    void MainMenuController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void MainMenuController::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void MainMenuController::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void MainMenuController::OnStart()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Play");
    }

    void MainMenuController::OnOption()
    {
        LOG_PRINT("OnOption");
    }

    void MainMenuController::OnCredit()
    {
        LOG_PRINT("OnCredit");
    }

    void MainMenuController::OnQuit()
    {
        LOG_PRINT("OnQuit");
    }
}