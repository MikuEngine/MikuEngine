#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIImage;
    class UIProgressBar;
    class UIText;
}

namespace game
{
    class PlayerControllerScript;

    class HUDController :
        public engine::Script<HUDController>
    {
        REGISTER_SCRIPT(HUDController, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        PlayerControllerScript* m_playerScript = nullptr;

        engine::UIText* m_currencyRubyText = nullptr;
        engine::UIText* m_currencySapphireText = nullptr;
        engine::UIText* m_currencyEmeraldText = nullptr;

        engine::UIProgressBar* m_fragileGaugeProgress = nullptr;

        engine::UIImage* m_hitImage = nullptr;
        engine::UIImage* m_fragileImage = nullptr;
    };
}