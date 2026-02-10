#include "GamePCH.h"
#include "SceneController_Tutorial.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>

#include <Framework/System/SoundSystem.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include <Framework/Object/Component/UI/UIImage.h>

#include "Manager/TimeScaler.h"
#include "Manager/StageManager.h"
#include <Manager/LoadingScreenDrawer.h>
#include <Script/AimModeController.h>
#include <Script/CharacterScript/Player/PlayerControllerScript.h>
#include <Manager/MessageCatalog.h>

#include <Script/UI/UIPopUpAnimator.h>
#include <Script/UI/UIMessageQueue.h>

#include "Scene/GameScene.h"
#include "Script/Scene/TutorialController.h"

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

    void SceneController_Tutorial::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        g_msg.Load("Resource/Data/Message/MessageTable.csv");

        if (auto* go = engine::GameObject::Find("UIMessageQueue"))
            if (auto* q = go->GetComponent<game::UIMessageQueue>())
                q->SetCatalog(&g_msg);

        // Buttons
        BindButton("UI_OpenMenu", [self = engine::Ptr<SceneController_Tutorial>(this)]() {if (self) self->OpenMenu(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Tutorial>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToPlay", [self = engine::Ptr<SceneController_Tutorial>(this)]() {if (self) self->BackToPlay(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Tutorial>(this)]() {if (self) self->CheckBackToTutorialLobby(true); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Tutorial>(this)]() {if (self) self->Back(); });

        // Sliders
        BindSlider("UI_BGMSlider", [self = engine::Ptr<SceneController_Tutorial>(this)](float v) {if (self) self->OnBGMChanged(v); });
        BindSlider("UI_SFXSlider", [self = engine::Ptr<SceneController_Tutorial>(this)](float v) {if (self) self->OnSFXChanged(v); });
        BindSlider("UI_SensitivitySlider", [self = engine::Ptr<SceneController_Tutorial>(this)](float v) {if (self) self->SetSensitivity(v); });
    }

    void SceneController_Tutorial::Start()
    {
        if (!TimeScaler::IsActive())
            TimeScaler::PlayWorld();

        auto* go = engine::GameObject::Find("Player");
        if (go)
        {
            m_aimMode = go->GetComponent<AimModeController>();
            m_playerScript = go->GetComponent<PlayerControllerScript>();     
        }

        if (auto* go = engine::GameObject::Find("HitImage"))
            m_hitImage = go->GetComponent<engine::UIImage>();

        if (m_playerScript)
        {
            m_playerScript->SetOnDamaged([this] {
                if (m_hitImage)
                {
                    m_hitImage->SetEffect(engine::UIEffectType::ScreenHit);

                    m_hitImage->SetEffectParam(0, { 1.0f, 0.0f, 0.0f, 0.0f });
                }

                // 튜토리얼에서 죽지않음
                float maxHp = m_playerScript->GetMaxHp();
                m_playerScript->SetCurrentHp(maxHp);

                // TODO: 사운드 있다면 여기에 추가
                });
        }

        m_menuPopUp = engine::GameObject::Find("Panel_Menu");
        if (m_menuPopUp) m_menuPopUp->SetActive(false);

        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_isMenuOpen = false;
        m_isOptionOpen = false;


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

    void SceneController_Tutorial::Update()
    {
        StageManager::Get().Update();

        if (m_hitImage)
        {
            auto params = m_hitImage->GetEffectParam(0);

            // 강도가 남아있다면 매 프레임 감소
            if (params.x > 0.0f)
            {
                params.x -= engine::Time::DeltaTime() * 2.5f; // 약 0.4초 동안 페이드 아웃
                if (params.x < 0.0f) params.x = 0.0f;

                m_hitImage->SetEffectParam(0, params);
            }
        }

        /*/ HUD: 런 재화 개수, 프레자일 게이지
        if (m_currencyRubyText)
            m_currencyRubyText->SetText(std::to_string(StageManager::Get().GetRunRuby()));
        if (m_currencySapphireText)
            m_currencySapphireText->SetText(std::to_string(StageManager::Get().GetRunSapphire()));
        if (m_currencyEmeraldText)
            m_currencyEmeraldText->SetText(std::to_string(StageManager::Get().GetRunEmerald()));
        //*/
        if (m_fragileGaugeProgress && m_playerScript)
        {
            float maxVal = m_playerScript->GetFragileGaugeMax();
            float current = m_playerScript->GetFragileGaugeCurrent();

            float t = (maxVal > 0.0f) ? (current / maxVal) : 0.0f;

            // 튜토리얼 전용 90프로 넘지 않도록 조정
            if (t > 0.9f)
            {
                t = 0.9f;
            }

            m_fragileGaugeProgress->SetValue(t);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            engine::SoundSystem::Get().PlayUI("UI_Click_Random");

            if (m_isOptionOpen) { Back(); return; }
            if (m_isMenuOpen) { SetMenuOpen(false); return; }

            SetMenuOpen(true);
        }


        /*//
        if (engine::Input::IsKeyPressed(engine::Keys::K))
        {
            if (auto* go = engine::GameObject::Find("UIMessageQueue"))
                if (auto* q = go->GetComponent<game::UIMessageQueue>())
                    q->PushMessageKey("Kill_001");
        }
        //*/
    }

    void SceneController_Tutorial::OnGui()
    {
    }

    void SceneController_Tutorial::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SceneController_Tutorial::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void SceneController_Tutorial::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick([cb]() {engine::SoundSystem::Get().PlayUI("UI_Click_Random");
        if (cb) cb(); });
    }

    void SceneController_Tutorial::BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* slider = go->GetComponent<engine::UISlider>();
        if (!slider) return;

        slider->SetOnValueChanged(std::move(cb));
    }

    void SceneController_Tutorial::SetMenuOpen(bool open)
    {
        m_isMenuOpen = open;

        const bool shouldStop = (m_isOptionOpen || m_isMenuOpen);
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

    void SceneController_Tutorial::OpenMenu()
    {
        SetMenuOpen(true);
    }

    void SceneController_Tutorial::OpenOption()
    {
        SetOptionOpen(true);
        SetMenuOpen(false);
    }

    void SceneController_Tutorial::BackToPlay()
    {
        SetMenuOpen(false);
    }

    void SceneController_Tutorial::CheckBackToTutorialLobby(bool open)
    {
        if (auto tutorial = engine::GameObject::Find("TutorialScript_Controller"))
        {
			tutorial->GetComponent<TutorialController>()->SetIndex(4, 1);
        }

        engine::SceneManager::Get().ChangeScene("10_PROTO_TutorialLobby");

        UpdateBlocker();
    }

    void SceneController_Tutorial::BackToMain()
    {
        GameScene::Change(SceneID::Main);
    }

    void SceneController_Tutorial::Back()
    {
        SetOptionOpen(false);
        SetMenuOpen(true);
    }

    void SceneController_Tutorial::UpdateBlocker()
    {
        if (!m_blocker) return;
        const bool isAnyPopupOpen = (m_isMenuOpen || m_isOptionOpen);

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

    void SceneController_Tutorial::OnBGMChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.bgm = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Tutorial::OnSFXChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.sfx = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Tutorial::SetSensitivity(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.controls.mouseSensitivity = SliderToSensitivity(v);

        app.SetUserSettings(s);
    }

    void SceneController_Tutorial::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;

        const bool shouldStop = (m_isOptionOpen || m_isMenuOpen);
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