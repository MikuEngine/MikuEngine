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
    }

    void TitleButtonController::Start()
    {
    }
    void TitleButtonController::Update()
    {
    }

    void TitleButtonController::OnGui()
    {
        if (ImGui::Button("Start"))
            StartGame();

        if (ImGui::Button("Option"))
            OpenOption();

        if (ImGui::Button("Credit"))
            OpenCredit();

        if (ImGui::Button("Quit"))
            QuitGame();
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
    }

    void TitleButtonController::OpenCredit()
    {
        LOG_PRINT("OpenCredit");
    }

    void TitleButtonController::QuitGame()
    {
        LOG_PRINT("QuitGame");
    }
}