#include "GamePCH.h"
#include "SceneController_Lobby.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>

#include <Framework/System/SoundSystem.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/RectTransform.h>
#include "Script/UI/UIPopUpAnimator.h"

#include "Manager/UpgradeProgressManager.h"
#include "Script/UpgradeController.h"
#include <Manager/LoadingScreenDrawer.h>


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

    void SceneController_Lobby::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        BindButton("UI_EnterPlay", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->EnterPlay(); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->OpenUpgrade(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToMain(); });
        BindButton("UI_BackToHub", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToHub(); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->Back(); });
        BindButton("UI_CloseButton_Upgrade", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->HandleEscape(); });

        BindButton("UI_EnterPlay", [self = engine::Ptr<SceneController_Lobby>(this)](bool) {if (self) self->ShowEffect(); });

        // Sliders
        BindSlider("UI_BGMSlider", [self = engine::Ptr<SceneController_Lobby>(this)](float v) {if (self) self->OnBGMChanged(v); });
        BindSlider("UI_SFXSlider", [self = engine::Ptr<SceneController_Lobby>(this)](float v) {if (self) self->OnSFXChanged(v); });
        BindSlider("UI_SensitivitySlider", [self = engine::Ptr<SceneController_Lobby>(this)](float v) {if (self) self->SetSensitivity(v); });
    }

    void SceneController_Lobby::Start()
    {
        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_groupSelect = engine::GameObject::Find("UIGroup_Select");
        if (m_groupSelect) m_groupSelect->SetActive(true);

        m_upgradePopUp = engine::GameObject::Find("UI_UpgradePopUp");
        if (m_upgradePopUp) m_upgradePopUp->SetActive(false);

        m_isOptionOpen = false;
        m_isUpgradeOpen = false;

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

        // Load
        auto* ugdGO = engine::GameObject::Find("UpgradeController");
        if (!ugdGO) return;

        auto* uc = ugdGO->GetComponent<game::UpgradeController>();
        if (!uc) return;

        game::UpgradeProgressManager::LoadProgress(*uc);
    }

    void SceneController_Lobby::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            HandleEscape();
        }
    }

    void SceneController_Lobby::OnGui()
    {

    }

    void SceneController_Lobby::Save(engine::json& j) const
    {
        Object::Save(j);
        j["OptionOpen"] = m_isOptionOpen;
        j["UpgradeOpen"] = m_isUpgradeOpen;
    }

    void SceneController_Lobby::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "OptionOpen", m_isOptionOpen);
        engine::JsonGet(j, "UpgradeOpen", m_isUpgradeOpen);
        SetOptionOpen(m_isOptionOpen);
        SetUpgradeOpen(m_isUpgradeOpen);
    }

    void SceneController_Lobby::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick([cb]() {engine::SoundSystem::Get().PlayUI("UI_Click_Random");
        if (cb) cb(); });
    }

    void SceneController_Lobby::BindButton(const std::string& name, engine::UIButton::HoverCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnHover(std::move(cb));
    }

    void SceneController_Lobby::BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* slider = go->GetComponent<engine::UISlider>();
        if (!slider) return;

        slider->SetOnValueChanged(std::move(cb));
    }

    void SceneController_Lobby::EnterPlay()
    {
        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        engine::SceneManager::Get().ChangeScene("Prototype_Play");
    }

    void SceneController_Lobby::OpenUpgrade()
    {
        SetUpgradeOpen(true);
    }

    void SceneController_Lobby::OpenOption()
    {
        SetOptionOpen(true);
    }

    void SceneController_Lobby::BackToMain()
    {
        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        engine::SceneManager::Get().ChangeScene("z_Hiro_Main");
    }

    void SceneController_Lobby::BackToHub()
    {
        SetUpgradeOpen(false);
    }

    void SceneController_Lobby::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;

        if (m_blocker)
            m_blocker->SetActive(open);

        if (!m_optionPopUp) return;
           
        if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
        {
            open ? anim->Open() : anim->Close();

            std::string soundName = m_isOptionOpen ? "UI_Open" : "UI_Close";
            engine::SoundSystem::Get().PlayUI(soundName);
        }
        else
        {
            m_optionPopUp->SetActive(open);
        }
    }

    void SceneController_Lobby::SetUpgradeOpen(bool open)
    {
        m_isUpgradeOpen = open;

        if (m_blocker)
            m_blocker->SetActive(open);

        if (!open)
        {
            auto* ugdGO = engine::GameObject::Find("UpgradeController");
            if (ugdGO)
            {
                if (auto* uc = ugdGO->GetComponent<game::UpgradeController>())
                {
                    // 또는 안전하게 함수를 호출 (추천)
                    uc->ResetSelection();
                }
            }
        }

        if (!m_upgradePopUp) return;

        if (auto* anim = m_upgradePopUp->GetComponent<game::UIPopUpAnimator>())
        {
            open ? anim->Open() : anim->Close();

            std::string soundName = m_isUpgradeOpen ? "UI_Open" : "UI_Close";
            engine::SoundSystem::Get().PlayUI(soundName);
        }
        else
        {
            m_upgradePopUp->SetActive(open);
        }
    }

    void SceneController_Lobby::HandleEscape()
    {
        if (m_isOptionOpen)
        {
            SetOptionOpen(false);
            engine::SoundSystem::Get().PlayUI("UI_Click_Random");
            return;
        }

        if (m_isUpgradeOpen)
        {
            SetUpgradeOpen(false);
            engine::SoundSystem::Get().PlayUI("UI_Click_Random");
            return;
        }
    }

    void SceneController_Lobby::Back()
    {
        engine::AppContext::GetApp().SaveUserSettings();
        SetOptionOpen(false);
    }

    void SceneController_Lobby::ShowEffect()
    {

    }

    void SceneController_Lobby::OnBGMChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.bgm = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Lobby::OnSFXChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.sfx = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Lobby::SetSensitivity(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.controls.mouseSensitivity = SliderToSensitivity(v);

        app.SetUserSettings(s);
    }
}