#include "GamePCH.h"
#include "SceneController_Play.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>

#include <Framework/System/SoundSystem.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include "Script/UI/UIPopUpAnimator.h"

#include "Manager/TimeScaler.h"
#include "Manager/StageManager.h"
#include <Manager/LoadingScreenDrawer.h>
#include <Script/AimModeController.h>
#include <Script/CharacterScript/Player/PlayerControllerScript.h>
#include <Manager/MessageCatalog.h>
#include <Script/UI/UIMessageQueue.h>

#include "Scene/GameScene.h"

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

        static game::MessageCatalog g_msg;
    }

    void SceneController_Play::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        std::string currentScene = (engine::SceneManager::Get().GetScene()) ? engine::SceneManager::Get().GetScene()->GetName() : "";

        if (currentScene != "10_PROTO_Tutorial")
        {
            StageManager::Get().BeginStage(); // 아직 맵 프리팹이 없어서 스테이지 세팅 불가
        }

        g_msg.Load("Resource/Data/Message/MessageTable.csv");

        if (auto* go = engine::GameObject::Find("UIMessageQueue"))
            if (auto* q = go->GetComponent<game::UIMessageQueue>())
                q->SetCatalog(&g_msg);

        // Buttons
        BindButton("UI_OpenMenu", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenMenu(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToPlay", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToPlay(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->CheckBackToMain(true); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->Back(); });

        BindButton("OK_Button", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToLobby(); });
        BindButton("Cancel_Button", [self = engine::Ptr<SceneController_Play>(this)] {if (self) self->CheckBackToMain(false); });

        // If Dead
        BindButton("ToLobby_Button", [self = engine::Ptr<SceneController_Play>(this)] {if (self) self->BackToLobby(); });
        BindButton("Restart_Button", [self = engine::Ptr<SceneController_Play>(this)] {if (self) self->BackToRestart(); });

        // Sliders
        BindSlider("UI_BGMSlider", [self = engine::Ptr<SceneController_Play>(this)](float v) {if (self) self->OnBGMChanged(v); });
        BindSlider("UI_SFXSlider", [self = engine::Ptr<SceneController_Play>(this)](float v) {if (self) self->OnSFXChanged(v); });
        BindSlider("UI_SensitivitySlider", [self = engine::Ptr<SceneController_Play>(this)](float v) {if (self) self->SetSensitivity(v); });
    }

    void SceneController_Play::Start()
    {
        if (!TimeScaler::IsActive())
            TimeScaler::PlayWorld();

        auto* go = engine::GameObject::Find("Player");
        if (go)
        {
            m_aimMode = go->GetComponent<AimModeController>();
            m_playerScript = go->GetComponent<PlayerControllerScript>();
        }

        m_menuPopUp = engine::GameObject::Find("Panel_Menu");
        if (m_menuPopUp) m_menuPopUp->SetActive(false);

        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_realGiveupPopUp = engine::GameObject::Find("UI_RealGiveupPopUp");
        if (m_realGiveupPopUp) m_realGiveupPopUp->SetActive(false);

        m_failPanel = engine::GameObject::Find("Panel_Fail");
        if (m_failPanel) m_failPanel->SetActive(false);

        m_isMenuOpen = false;
        m_isOptionOpen = false;
        m_isDead = false;

        if (!m_isDead) TimeScaler::PlayWorld();

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

        // HUD: Currency counts (Canvas_HUD > Currency > Ruby/Sapphire/Emerald > * Count)
        if (auto* go = engine::GameObject::Find("Ruby Count"))
            m_currencyRubyText = go->GetComponent<engine::UIText>();
        if (auto* go = engine::GameObject::Find("Sapphire Count"))
            m_currencySapphireText = go->GetComponent<engine::UIText>();
        if (auto* go = engine::GameObject::Find("Emerald Count"))
            m_currencyEmeraldText = go->GetComponent<engine::UIText>();
        // Fragile Gauge (Canvas_HUD > Fragile Gauge > Fragile Gauge Progress)
        if (auto* go = engine::GameObject::Find("Fragile Gauge Progress"))
            m_fragileGaugeProgress = go->GetComponent<engine::UIProgressBar>();
    }

    void SceneController_Play::Update()
    {
        StageManager::Get().Update();

        // HUD: 런 재화 개수, 프레자일 게이지
        if (m_currencyRubyText)
            m_currencyRubyText->SetText(std::to_string(StageManager::Get().GetRunRuby()));
        if (m_currencySapphireText)
            m_currencySapphireText->SetText(std::to_string(StageManager::Get().GetRunSapphire()));
        if (m_currencyEmeraldText)
            m_currencyEmeraldText->SetText(std::to_string(StageManager::Get().GetRunEmerald()));
        if (m_fragileGaugeProgress && m_playerScript)
        {
            float maxVal = m_playerScript->GetFragileGaugeMax();
            float t = (maxVal > 0.0f) ? (m_playerScript->GetFragileGaugeCurrent() / maxVal) : 0.0f;
            m_fragileGaugeProgress->SetValue(t);
        }

        // 플레이어 사망 시 실패 (HP 0, 프레자일 100%, 또는 Dead 상태)
        if (!m_isDead && m_playerScript)
        {
            bool fragileFull = m_playerScript->GetFragileGaugeCurrent() >= m_playerScript->GetFragileGaugeMax();
            bool hpZero = m_playerScript->GetCurrentHp() <= 0.0f;
            bool playerDeadState = (m_playerScript->GetCurrentState() == "Dead");
            if (fragileFull || hpZero || playerDeadState)
                Fail();
        }

        if (!m_isDead && engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            engine::SoundSystem::Get().PlayUI("UI_Click_Random");

            if (m_isOptionOpen) { Back(); return; }
            if (m_isGiveupOpen) { CheckBackToMain(false); return; }
            if (m_isMenuOpen) { SetMenuOpen(false); return; }

            SetMenuOpen(true);
        }

        // Debug
        if (engine::Input::IsKeyPressed(engine::Keys::F4))
        {
            Fail();
        }

        if (engine::Input::IsKeyPressed(engine::Keys::K))
        {
            if (auto* go = engine::GameObject::Find("UIMessageQueue"))
                if (auto* q = go->GetComponent<game::UIMessageQueue>())
                    q->PushMessageKey("Kill_001");
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

        button->AddOnClick([cb]() {engine::SoundSystem::Get().PlayUI("UI_Click_Random");
        if (cb) cb(); });
    }

    void SceneController_Play::BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* slider = go->GetComponent<engine::UISlider>();
        if (!slider) return;

        slider->SetOnValueChanged(std::move(cb));
    }

    void SceneController_Play::SetMenuOpen(bool open)
    {
        m_isMenuOpen = open;

        const bool shouldStop = (m_isOptionOpen || m_isMenuOpen || m_isGiveupOpen || m_isDead);
        if (shouldStop) TimeScaler::StopWorld();
        else            TimeScaler::PlayWorld();

        if (m_aimMode) m_aimMode->SetPaused(shouldStop);

        if (m_menuPopUp)
        {
            if (auto* anim = m_menuPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();

                std::string soundName = open ? "UI_Open" : "UI_Close";
                engine::SoundSystem::Get().PlayUI(soundName);
            }
            else
            {
                m_menuPopUp->SetActive(open);
            }
        }

        UpdateBlocker();
    }

    void SceneController_Play::OpenMenu()
    {
        SetMenuOpen(true);
    }

    void SceneController_Play::OpenOption()
    {
        SetOptionOpen(true);
        SetMenuOpen(false);
    }

    void SceneController_Play::BackToPlay()
    {
        SetMenuOpen(false);
    }

    void SceneController_Play::CheckBackToMain(bool open)
    {
        m_isGiveupOpen = open;
        m_realGiveupPopUp->SetActive(open);

        UpdateBlocker();
    }

    void SceneController_Play::BackToMain()
    {
        GameScene::Change(SceneID::Main);
    }

    void SceneController_Play::BackToLobby()
    {
        GameScene::Change(SceneID::Lobby);
    }

    void SceneController_Play::BackToRestart()
    {
        GameScene::Change(SceneID::Play);
    }

    void SceneController_Play::Back()
    {
        SetOptionOpen(false);
        SetMenuOpen(true);
    }

    void SceneController_Play::Fail()
    {
        m_isDead = true;
        if (m_failPanel)
            m_failPanel->SetActive(true);

        const bool isAnyPopupOpen = (m_isMenuOpen || m_isOptionOpen || m_isGiveupOpen || m_isDead);

        if (isAnyPopupOpen)
            TimeScaler::StopWorld();
        else
            TimeScaler::PlayWorld();

        auto* aimGO = engine::GameObject::Find("Player");

        if (aimGO)
        {
            if (auto* amc = aimGO->GetComponent<AimModeController>())
            {
                amc->SetPaused(isAnyPopupOpen);
            }
        }
    }

    void SceneController_Play::UpdateBlocker()
    {
        if (!m_blocker) return;
        const bool isAnyPopupOpen = (m_isMenuOpen || m_isOptionOpen || m_isGiveupOpen || m_isDead);

        m_blocker->SetActive(isAnyPopupOpen);

        if (isAnyPopupOpen)
            TimeScaler::StopWorld();
        else
            TimeScaler::PlayWorld();

        auto* aimGO = engine::GameObject::Find("Player");

        if (aimGO)
        {
            if (auto* amc = aimGO->GetComponent<AimModeController>())
            {
                amc->SetPaused(isAnyPopupOpen);
            }
        }
    }

    void SceneController_Play::OnBGMChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.bgm = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Play::OnSFXChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.sfx = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Play::SetSensitivity(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.controls.mouseSensitivity = SliderToSensitivity(v);

        app.SetUserSettings(s);
    }

    void SceneController_Play::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;

        const bool shouldStop = (m_isOptionOpen || m_isMenuOpen || m_isGiveupOpen || m_isDead);
        if (shouldStop) TimeScaler::StopWorld();
        else            TimeScaler::PlayWorld();

        if (m_aimMode) m_aimMode->SetPaused(shouldStop);

        if (m_optionPopUp)
        {
            if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();

                std::string soundName = open ? "UI_Open" : "UI_Close";
                engine::SoundSystem::Get().PlayUI(soundName);
            }
            else
            {
                m_optionPopUp->SetActive(open);
            }
        }

        UpdateBlocker();
    }
}