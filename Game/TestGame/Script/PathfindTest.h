#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class PathfindingAgent;
    class GameObject;
}

namespace game
{
    class PathfindTest :
        public engine::Script<PathfindTest>
    {
        REGISTER_COMPONENT(PathfindTest, Script)

    private:
        engine::PathfindingAgent* m_agent = nullptr;
        engine::GameObject* m_target = nullptr;
        float m_moveSpeed = 3.0f;

    public:
        //void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}