#pragma once
#include <Framework/Object/Component/Script.h>
#include <Manager/StageManager.h>

namespace engine
{
    class UIText;
}

namespace game
{
    class UICurrentStageTitle :
        public engine::Script<UICurrentStageTitle>
    {
        REGISTER_SCRIPT(UICurrentStageTitle, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void TimeCheck();

    private:
        // GameObject
        engine::UIText* m_titleText = nullptr;

        engine::Vector4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };

        int m_stageNumber = 1;
        float m_elapsed = 0.0f;
        float m_fadeDuration = 3.0f;
        float m_fadeDelay = 0.2f;
        bool m_done = false;
    };
}