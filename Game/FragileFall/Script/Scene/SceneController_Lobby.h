#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>

namespace engine
{
    class UIScrollView;
}

namespace game
{
    class LobbyInteraction;

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
        void ShowEffect(const std::string& targetName, bool hovered);

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

        bool m_entered = false;

        bool m_isOptionOpen = false;
        bool m_isUpgradeOpen = false;

        bool m_isPlayerMove = false;
        bool m_walkStarted = false;

        //float m_moveElapsed = 0.0f;
        //float m_moveDuration = 0.9f;

        float m_rotElapsed = 0.0f;
        float m_moveElapsed = 0.0f;
        float m_rotDuration = 0.6f;   // 회전 0.6초
        float m_moveDuration = 2.5f; // 이동 2.5초
        bool  m_rotDone = false;

        engine::Vector3 m_moveStartPos = { 0,0,0 };
        engine::Vector3 m_moveTargetPos = { 10.382f, 0.270f, 29.869f };

        float m_moveStartYaw = 0.0f;
        float m_moveTargetYaw = 0.0f;

        engine::Quaternion m_moveStartRot = engine::Quaternion::Identity;
        engine::Quaternion m_moveTargetRot = engine::Quaternion::Identity;

        bool m_upgradeTransition = false;
        bool m_optionTransition = false;
        float m_uiTransitionTimer = 0.0f;

        engine::Vector3 m_curveP0 = { 0,0,0 };
        engine::Vector3 m_curveP1 = { 0,0,0 };
        engine::Vector3 m_curveP2 = { 0,0,0 };

        // 휘는 정도
        float m_curve01 = 0.95f;

    private:
        // GameObject
        engine::GameObject* m_optionPopUp = nullptr;
        engine::GameObject* m_blocker = nullptr;
        engine::GameObject* m_groupSelect = nullptr;
        engine::GameObject* m_upgradePopUp = nullptr;
        engine::GameObject* m_playerPreview = nullptr;

        engine::UIScrollView* m_upgradeScroll = nullptr;

        LobbyInteraction* m_interaction;
    };
}