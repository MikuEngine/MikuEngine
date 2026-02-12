#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIText;
}

enum class RunCurrencyType
{
    Ruby,
    Sapphire,
    Emerald
};

namespace game
{
    class UICurrencyAnimator :
        public engine::Script<UICurrencyAnimator>
    {
        REGISTER_SCRIPT(UICurrencyAnimator, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void SetTargetValue(engine::UIText* text, int v);
        void AddDelta(engine::UIText* text, int delta);

        float m_countSpeed = 10.0f;

        // 스케일 펄스
        bool  m_enablePulse = true;
        float m_pulseScale = 1.02f;          // 1.01~1.02 권장
        float m_pulseDuration = 0.12f;       // 초 (짧게)

        // 색상 플래시
        bool  m_enableColorFlash = true;
        float m_colorFlashDuration = 0.15f;  // 초
        engine::Vector4 m_gainColor = engine::Vector4(0.30f, 1.00f, 0.50f, 1.00f);
        engine::Vector4 m_lossColor = engine::Vector4(1.00f, 0.35f, 0.35f, 1.00f);

        RunCurrencyType m_type = RunCurrencyType::Ruby;

    private:
        void ApplyText(engine::UIText* text, int v);

        void TriggerPulse();
        void TriggerColorFlash(int delta);
        void UpdatePulse(float dt);
        void UpdateColorFlash(float dt);

        static engine::Vector4 LerpColor(const engine::Vector4& a, const engine::Vector4& b, float t);
        static float Clamp01(float v);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        engine::UIText* m_rubyCount = nullptr;
        engine::UIText* m_sapphireCount = nullptr;
        engine::UIText* m_emeraldCount = nullptr;
        engine::RectTransform* m_rt = nullptr;

        int   m_displayValue = 0;
        int   m_targetValue = 0;
        bool  m_initialized = false;

        int m_targetRuby = 0;
        int m_targetSapphire = 0;
        int m_targetEmerald = 0;

        int m_displayRuby = 0;
        int m_displaySapphire = 0;
        int m_displayEmerald = 0;

        // 카운트 누적용
        float m_accRuby = 0.0f;
        float m_accSapphire = 0.0f;
        float m_accEmerald = 0.0f;

        engine::Vector2 m_baseScale = engine::Vector2(1.0f, 1.0f);
        engine::Vector4 m_baseColor = engine::Vector4(1.0f, 1.0f, 1.0f, 1.0f);

        // effect timers
        float m_pulseT = 0.0f;               // 0이면 비활성
        float m_colorT = 0.0f;               // 0이면 비활성
        engine::Vector4 m_flashColor = engine::Vector4(1, 1, 1, 1);
    };
}