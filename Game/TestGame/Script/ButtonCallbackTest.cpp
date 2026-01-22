#include "GamePCH.h"
#include "ButtonCallbackTest.h"

#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void ButtonCallbackTest::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_StartButton", [self = engine::Ptr<ButtonCallbackTest>(this)]() {if (self) self->OnStart();});
        BindButton("UI_OptionButton", [self = engine::Ptr<ButtonCallbackTest>(this)]() {if (self) self->OnOption();});
        BindButton("UI_CreditButton", [self = engine::Ptr<ButtonCallbackTest>(this)]() {if (self) self->OnCredit();});
        BindButton("UI_QuitButton", [self = engine::Ptr<ButtonCallbackTest>(this)]() {if (self) self->OnQuit();});
    }

    void ButtonCallbackTest::Start()
    {
    }
    void ButtonCallbackTest::Update()
    {
    }
    void ButtonCallbackTest::OnGui()
    {
    }

    void ButtonCallbackTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void ButtonCallbackTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void ButtonCallbackTest::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;
        auto* go = scene->FindGameObject(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void ButtonCallbackTest::CallThis()
    {
        LOG_PRINT("CallThis");
    }

    void ButtonCallbackTest::OnStart()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Play");
    }

    void ButtonCallbackTest::OnOption()
    {
        LOG_PRINT("OnOption");
    }

    void ButtonCallbackTest::OnCredit()
    {
        LOG_PRINT("OnCredit");
    }

    void ButtonCallbackTest::OnQuit()
    {
        LOG_PRINT("OnQuit");
    }
}