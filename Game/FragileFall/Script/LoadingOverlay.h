#pragma once

#include <Framework/Object/Component/Script.h>

#include <Framework/Object/Component/UI/UIProgressBar.h>

namespace engine
{
    class UIProgressBar;
}

namespace game
{
    class LoadingOverlay :
        public engine::Script<LoadingOverlay>
    {
        REGISTER_SCRIPT(LoadingOverlay, Script)

    public:
        void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void Show();
        void Hide();
        void SetProgress(float t);

    private:
        engine::UIProgressBar* m_bar = nullptr;
        float m_progress = 0.0f;
        bool m_visible = false;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}