#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UIPopUpAnimator :
        public engine::Script<UIPopUpAnimator>
    {
        REGISTER_COMPONENT(UIPopUpAnimator, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

        void Open();
        void Close();

    private:
        engine::Vector2 m_openPos{ 0.0f, 0.0f };
        engine::Vector2 m_closedPos{ 0.0f, -300.0f };

        float m_time = 0.0f;
        float m_duration = 0.25f;

        bool m_isOpen = false;
        bool m_animating = false;
        bool m_opening = false;
    };
}