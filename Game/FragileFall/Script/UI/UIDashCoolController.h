#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>

namespace engine
{
    class GameObject;
    class UIProgressBar;
    class UIImage;
    class Transform;
}

namespace game
{
    class PlayerControllerScript;

    class UIDashCoolController :
        public engine::Script<UIDashCoolController>
    {
        REGISTER_SCRIPT(UIDashCoolController, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void ResolveReferences();
        void UpdateDashUI();
        void RefreshUIImageCacheIfNeeded();
        void ApplyGlobalAlphaToUIImages(float alpha01);
        void ApplyUniformScaleToTargets();
        void TriggerFillFlashByRecoveredDashCount(int recoveredDashCount);
        void UpdateFillFlash(float deltaTime);
        void ApplyProgressBarColors();
        bool IsPlayerDead() const;

    private:
        static constexpr int kDashSlotCount = 3;

        std::string m_playerObjectName = "Player";
        std::string m_dashCoolObjectName = "DashCool";
        std::string m_fillObjectName = "Fill";
        std::string m_bgCaseName = "BG_Case";
        std::string m_bgDashBtnName = "BG_Dash_Btn";
        std::string m_ringGauge1Name = "RingGauge_1";
        std::string m_ringGauge2Name = "RingGauge_2";
        std::string m_ringGauge3Name = "RingGauge_3";

        float m_childUniformScale = 1.0f;

        float m_fullDashFadeDelay = 3.0f;
        float m_fadeOutDuration = 1.0f;
        float m_fadeInDuration = 0.4f;
        float m_playerDeathFadeOutDuration = 2.0f;

        float m_fillFlashDuration = 0.2f;
        bool m_enableFillFlash = false;
        engine::Vector4 m_fillFlashColorDash1 = engine::Vector4(1.0f, 0.2f, 0.2f, 1.0f);
        engine::Vector4 m_fillFlashColorDash2 = engine::Vector4(1.0f, 0.9f, 0.2f, 1.0f);
        engine::Vector4 m_fillFlashColorDash3 = engine::Vector4(0.45f, 0.85f, 1.0f, 1.0f);

        engine::Ptr<PlayerControllerScript> m_playerScript;
        engine::Ptr<engine::GameObject> m_dashCoolObject;
        engine::Ptr<engine::UIProgressBar> m_fillProgressBar;
        engine::Ptr<engine::GameObject> m_fillObject;
        engine::Ptr<engine::UIImage> m_fillImage;
        engine::Ptr<engine::UIImage> m_fillBackgroundImage;
        engine::Ptr<engine::GameObject> m_bgCaseObject;
        engine::Ptr<engine::GameObject> m_bgDashBtnObject;
        engine::Ptr<engine::GameObject> m_ringGauge1;
        engine::Ptr<engine::GameObject> m_ringGauge2;
        engine::Ptr<engine::GameObject> m_ringGauge3;

        std::vector<engine::Ptr<engine::UIImage>> m_uiImages;
        std::vector<float> m_uiImageBaseAlphas;
        engine::Ptr<engine::GameObject> m_lastUIImageCacheRoot;

        bool m_scaleBasesCached = false;
        engine::Vector3 m_bgCaseBaseScale = engine::Vector3::One;
        engine::Vector3 m_bgDashBtnBaseScale = engine::Vector3::One;
        engine::Vector3 m_ringGauge1BaseScale = engine::Vector3::One;
        engine::Vector3 m_ringGauge2BaseScale = engine::Vector3::One;
        engine::Vector3 m_ringGauge3BaseScale = engine::Vector3::One;

        int m_prevDashCount = -1;
        float m_fullDashElapsed = 0.0f;
        float m_targetAlpha = 1.0f;
        float m_currentAlpha = 1.0f;

        bool m_fillFlashPlaying = false;
        float m_fillFlashElapsed = 0.0f;
        engine::Vector4 m_fillDefaultColor = engine::Vector4(1, 1, 1, 1);
        engine::Vector4 m_fillFlashTargetColor = engine::Vector4(1, 1, 1, 1);
        engine::Vector4 m_fillVisualColor = engine::Vector4(1, 1, 1, 1);
        engine::Vector4 m_progressBackgroundBaseColor = engine::Vector4(1, 1, 1, 0);
        bool m_holdFillValueDuringFlash = false;
        float m_heldFillValue = 1.0f;

        bool m_playerDeathFadeStarted = false;
    };
}
