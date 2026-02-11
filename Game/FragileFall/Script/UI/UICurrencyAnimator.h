#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIText;
}

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
        void SetTargetValue(int v);
        void AddDelta(int delta);

        float m_countSpeed = 600.0f;

    private:
        void ApplyText(int v);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        engine::UIText* m_text = nullptr;

        int   m_displayValue = 0;
        int   m_targetValue = 0;
        bool  m_initialized = false;
    };
}