#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>

namespace engine
{
    class UIText;
    class UIProgressBar;
}

namespace game
{
    class AimModeController;
    class PlayerControllerScript;

    class SceneController_Play :
        public engine::Script<SceneController_Play>
    {
        REGISTER_SCRIPT(SceneController_Play, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        // Bind
        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);
        void BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb);

        void SetMenuOpen(bool open);
        void SetOptionOpen(bool open);
        void CheckBackToMain(bool open);

        void OpenMenu();

        void OpenOption();
        void BackToPlay();
        void BackToMain();
        void BackToLobby();
        void BackToRestart();

        void Back();

        void Fail();

        void UpdateBlocker();

        // Slider
        void OnBGMChanged(float v);
        void OnSFXChanged(float v);
        void SetSensitivity(float v);

        bool m_bound = false;
        bool m_isMenuOpen = false;
        bool m_isOptionOpen = false;
        bool m_isGiveupOpen = false;

        bool m_isDead = false;

    private:
        std::string msgPath = "Resource/Data/Message/MessageTable.csv";

    private:
        // GameObject
        engine::GameObject* m_menuPopUp = nullptr;
        engine::GameObject* m_optionPopUp = nullptr;
        engine::GameObject* m_blocker = nullptr;

        engine::GameObject* m_realGiveupPopUp = nullptr;
        engine::GameObject* m_failPanel = nullptr;

        AimModeController* m_aimMode = nullptr;
        PlayerControllerScript* m_playerScript = nullptr;

        // HUD: Canvas_HUD > Currency > Ruby/Sapphire/Emerald > * Count (UIText), Fragile Gauge > Fragile Gauge Progress (UIProgressBar)
        engine::UIText* m_currencyRubyText = nullptr;
        engine::UIText* m_currencySapphireText = nullptr;
        engine::UIText* m_currencyEmeraldText = nullptr;
        engine::UIProgressBar* m_fragileGaugeProgress = nullptr;
    };
}