#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UIPopUpAnimator :
        public engine::Script<UIPopUpAnimator>
    {
        REGISTER_SCRIPT(UIPopUpAnimator, Script)

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
        engine::RectTransform* m_rt = nullptr;

        engine::Vector2 m_visible{ 0.0f, 200.0f };
        engine::Vector2 m_hidden{ 0.0f, 900.0f };

        float m_time = 0.0f;
        float m_duration = 0.15f;

        bool m_isOpen = false;
        bool m_animating = false;
        bool m_opening = false;

        bool m_inited = false;
    };
}