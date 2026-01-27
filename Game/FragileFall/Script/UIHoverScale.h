#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class RectTransform;
    class UIButton;
}

namespace game
{
    class UIHoverScale :
        public engine::Script<UIHoverScale>
    {
        REGISTER_SCRIPT(UIHoverScale, Script)

    public:
        void Awake() override;
        //void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void Apply();

    private:
        engine::RectTransform* m_rt = nullptr;
        engine::UIButton* m_btn = nullptr;
        engine::Vector2 m_normalScale = { 1.0f, 1.0f };
        engine::Vector2 m_baseSize = { 100.0f, 100.0f };

        float m_hoverScale = 1.1f;

        float m_speed = 12.0f;

        bool m_hovered = false;
    };
}