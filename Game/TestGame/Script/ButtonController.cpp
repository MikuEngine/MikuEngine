#include "GamePCH.h"
#include "ButtonController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void ButtonController::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_StartButton", [self = engine::Ptr<ButtonController>(this)]() {if (self) self->OnStart();});
        BindButton("UI_OptionButton", [self = engine::Ptr<ButtonController>(this)]() {if (self) self->OnOption();});
        BindButton("UI_CreditButton", [self = engine::Ptr<ButtonController>(this)]() {if (self) self->OnCredit();});
        BindButton("UI_QuitButton", [self = engine::Ptr<ButtonController>(this)]() {if (self) self->OnQuit();});
    }

    void ButtonController::Start()
    {
    }
    void ButtonController::Update()
    {
    }

    void ButtonController::OnGui()
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

    void ButtonController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void ButtonController::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void ButtonController::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void ButtonController::OnStart()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Play");
    }

    void ButtonController::OnOption()
    {
        LOG_PRINT("OnOption");
    }

    void ButtonController::OnCredit()
    {
        LOG_PRINT("OnCredit");
    }

    void ButtonController::OnQuit()
    {
        LOG_PRINT("OnQuit");
    }
}