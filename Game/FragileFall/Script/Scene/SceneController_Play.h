#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>

namespace engine
{
    class UIText;
    class UIProgressBar;
    class UIImage;
    class SkeletalAnimator;
}

namespace game
{
    class AimModeController;
    class PlayerControllerScript;
    class CameraEffectScript;

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
        void BindButton(const std::string& name, engine::UIButton::HoverCallback cb);
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
        bool m_autoReturnToLobbyOnFail = true;
        float m_failAutoReturnDelaySec = 3.0f;
        float m_failElapsedSec = 0.0f;
        bool m_failReturnTriggered = false;

        // ─── 사망 연출 ───
        bool  m_deathCinematicActive = false;  // 사망 연출 진행 중
        float m_deathCinematicDuration = 2.0f; // 연출 총 길이 (GUI 조정 가능)
        float m_deathCinematicElapsed = 0.0f;
        bool  m_deathCinematicDone = false;     // 연출 완료 후 FailPanel 표시 플래그
        float m_deathCameraApproachRatio = 0.45f; // 0~1: 카메라가 플레이어 쪽으로 접근하는 비율
        float m_deathCameraTargetYOffset = 1.5f;  // 카메라 목표 Y = player.y + offset
        float m_deathCameraMoveSpeed = 1.8f;      // 카메라 워킹 속도 배율 (1=기본, 클수록 빨리 접근)
        engine::Vector3 m_deathCameraLookAtOffset = engine::Vector3(0.0f, 1.0f, 0.0f); // 플레이어 기준 시선 오프셋
        bool  m_deathCrystalBurstTriggered = false;
        std::string m_deathCrystalColorSuffix = "Gray";

    private:
        std::string msgPath = "Resource/Data/Message/MessageTable.csv";

    private:
        // GameObject
        engine::GameObject* m_menuPopUp = nullptr;
        engine::GameObject* m_optionPopUp = nullptr;
        engine::GameObject* m_blocker = nullptr;

        engine::GameObject* m_realGiveupPopUp = nullptr;
        engine::GameObject* m_failPanel = nullptr;
        engine::GameObject* m_deathCrystalInstance = nullptr;

        AimModeController* m_aimMode = nullptr;
        PlayerControllerScript* m_playerScript = nullptr;
        CameraEffectScript* m_cameraEffect = nullptr;
        engine::SkeletalAnimator* m_playerAnimator = nullptr;
        engine::GameObject* m_mainCamera = nullptr;
        engine::Vector3 m_deathCameraStartPos = engine::Vector3::Zero;

        // HUD: Canvas_HUD > Currency > Ruby/Sapphire/Emerald > * Count (UIText), Fragile Gauge > Fragile Gauge Progress (UIProgressBar)
    };
}