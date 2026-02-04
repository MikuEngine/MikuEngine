#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>

namespace game
{
    class SceneController_Lobby :
        public engine::Script<SceneController_Lobby>
    {
        REGISTER_SCRIPT(SceneController_Lobby, Script)
    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);
        void BindButton(const std::string& name, engine::UIButton::HoverCallback cb);
        void BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb);

        // OnClick
        void EnterPlay();
        void OpenUpgrade();
        void OpenOption();
        void BackToMain();
        void BackToHub();

        void Back();

        // OnHover
        void ShowEffect();

        // Slider
        void OnBGMChanged(float v);
        void OnSFXChanged(float v);
        void SetSensitivity(float v);

        // State / UI Control
        void SetOptionOpen(bool open);
        void SetUpgradeOpen(bool open);
        void HandleEscape();

    private:
        bool m_bound = false;

        bool m_isOptionOpen = false;
        bool m_isUpgradeOpen = false;

        bool m_isPlayerMove = false;

        float m_moveElapsed = 0.0f;
        float m_moveDuration = 1.5f;

        engine::Vector3 m_moveStartPos = { 0,0,0 };
        engine::Vector3 m_moveTargetPos = { 0,0,18 };

        engine::Quaternion m_moveStartRot = engine::Quaternion::Identity;
        engine::Quaternion m_moveTargetRot = engine::Quaternion::Identity;

    private:
        // GameObject
        engine::GameObject* m_optionPopUp = nullptr;
        engine::GameObject* m_blocker = nullptr;
        engine::GameObject* m_groupSelect = nullptr;
        engine::GameObject* m_upgradePopUp = nullptr;
        engine::GameObject* m_playerPreview = nullptr;
    };
}