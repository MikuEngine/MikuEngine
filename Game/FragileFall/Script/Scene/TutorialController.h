#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UIMessageQueue;

    class TutorialController :
        public engine::Script<TutorialController>
    {
        REGISTER_SCRIPT(TutorialController, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        engine::Ptr<UIMessageQueue> m_queue;

        std::vector<std::string> m_texts;
        int m_index = 0;

        void ShowCurrent();
        void Next();
    };
}