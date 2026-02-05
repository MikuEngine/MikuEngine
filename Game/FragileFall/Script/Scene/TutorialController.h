#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UIMessageQueue;

    struct TutorialStep
    {
        std::vector<std::string> pages; // 동시에 보여줄 메시지들
    };

    class TutorialController :
        public engine::Script<TutorialController>
    {
        REGISTER_SCRIPT(TutorialController, Script)

    private:
        UIMessageQueue* m_queue = nullptr;
        int m_stepIndex = 0;
        int m_pageIndex = 0;

        std::string m_nextDoorObjectName;
        engine::Ptr<engine::GameObject> m_nextDoorObject = nullptr;
        engine::Vector3 m_doorPosition = { 0, 0, 0 };

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;



        void ShowPage();
        void Next();
        void Prev();
    };
}