#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIText;
}

namespace game
{
    class UISliderValueTextBinder :
        public engine::Script<UISliderValueTextBinder>
    {
        REGISTER_SCRIPT(UISliderValueTextBinder, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void Refresh(bool force);

    private:
        engine::UIText* m_volumeTxt = nullptr;
        engine::UIText* m_SFXTxt = nullptr;
        engine::UIText* m_SensiTxt = nullptr;
    };
}