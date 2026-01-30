#include "GamePCH.h"
#include "SceneController_Main.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>

#include <Framework/System/SoundSystem.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include "UIPopUpAnimator.h"

namespace game
{
    namespace
    {
        // 마우스 감도 예외처리
        constexpr float kSensMin = 0.2f;
        constexpr float kSensMax = 3.0f;

        static float Clamp(float v, float a, float b)
        {
            if (v < a) return a;
            if (v > b) return b;
            return v;
        }

        static float SliderToSensitivity(float t01)
        {
            t01 = Clamp(t01, 0.f, 1.f);
            return kSensMin + t01 * (kSensMax - kSensMin);
        }

        // sensitivity -> slider(0~1)
        static float SensitivityToSlider(float sens)
        {
            sens = Clamp(sens, kSensMin, kSensMax);
            return (sens - kSensMin) / (kSensMax - kSensMin);
        }
    }

    void SceneController_Main::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        // Buttons
        BindButton("UI_StartButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->StartGame();});
        BindButton("UI_OptionButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->OpenOption();});
        BindButton("UI_CreditButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->OpenCredit();});
        BindButton("UI_QuitButton", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->QuitGame();});
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->Back();});
        BindButton("UI_CloseButton_Credit", [self = engine::Ptr<SceneController_Main>(this)]() {if (self) self->Back();});

        // Sliders
        BindSlider("UI_BGMSlider", [self = engine::Ptr<SceneController_Main>(this)](float v) {if (self) self->OnBGMChanged(v); });
        BindSlider("UI_SFXSlider", [self = engine::Ptr<SceneController_Main>(this)](float v) {if (self) self->OnSFXChanged(v); });
        BindSlider("UI_SensitivitySlider", [self = engine::Ptr<SceneController_Main>(this)](float v) {if (self) self->SetSensitivity(v); });
    }

    void SceneController_Main::Start()
    {
        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_creditPopUp = engine::GameObject::Find("UI_CreditPopUp");
        if (m_creditPopUp) m_creditPopUp->SetActive(false);
       
        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_isOptionOpen = false;
        m_isCreditOpen = false;

        auto& app = engine::AppContext::GetApp();
        const auto& s = app.GetUserSettings();

        if (auto* go = engine::GameObject::Find("UI_BGMSlider"))
            if (auto* slider = go->GetComponent<engine::UISlider>())
                slider->SetValue(s.audio.bgm, false);

        if (auto* go = engine::GameObject::Find("UI_SFXSlider"))
            if (auto* slider = go->GetComponent<engine::UISlider>())
                slider->SetValue(s.audio.sfx, false);

        if (auto* go = engine::GameObject::Find("UI_SensitivitySlider"))
            if (auto* slider = go->GetComponent<engine::UISlider>())
                slider->SetValue(SensitivityToSlider(s.controls.mouseSensitivity), false);
    }

    void SceneController_Main::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            if (m_isOptionOpen || m_isCreditOpen)
                Back();
        }
    }

    void SceneController_Main::OnGui()
    {

    }

    void SceneController_Main::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SceneController_Main::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void SceneController_Main::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick([cb]() {engine::SoundSystem::Get().PlayUI("UI_Click_Random");
        if (cb) cb(); });
    }

    void SceneController_Main::BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* slider = go->GetComponent<engine::UISlider>();
        if (!slider) return;

        slider->SetOnValueChanged(std::move(cb));
    }

    void SceneController_Main::StartGame()
    {
        engine::SceneManager::Get().ChangeScene("z_Hiro_Lobby");
    }

    void SceneController_Main::OpenOption()
    {
        LOG_PRINT("OpenOption");

        SetOptionOpen(true);
        SetCreditOpen(false);
    }

    void SceneController_Main::OpenCredit()
    {
        LOG_PRINT("OpenCredit");

        SetCreditOpen(true);
        SetOptionOpen(false);
    }

    void SceneController_Main::QuitGame()
    {
        LOG_PRINT("QuitGame");
    }

    void SceneController_Main::Back()
    {
        engine::AppContext::GetApp().SaveUserSettings();
        SetOptionOpen(false);
        SetCreditOpen(false);
    }

    void SceneController_Main::OnBGMChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.bgm = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Main::OnSFXChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.sfx = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Main::SetSensitivity(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.controls.mouseSensitivity = SliderToSensitivity(v);
        
        app.SetUserSettings(s);
    }

    void SceneController_Main::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;
        if (m_optionPopUp)
        {
            if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();
            }
        }

        UpdateBlocker();
    }

    void SceneController_Main::SetCreditOpen(bool open)
    {
        m_isCreditOpen = open;
        if (m_creditPopUp)
        {
            if (auto* anim = m_creditPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();
            }
        }

        UpdateBlocker();
    }

    void SceneController_Main::UpdateBlocker()
    {
        if (!m_blocker) return;
        m_blocker->SetActive(m_isOptionOpen || m_isCreditOpen);
    }
}