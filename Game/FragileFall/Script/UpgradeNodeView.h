#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UpgradeNodeView :
        public engine::Script<UpgradeNodeView>
    {
        REGISTER_COMPONENT(UpgradeNodeView, Script)

    public:
        void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

        void SetVisualState(bool unlocked, bool purchased);

    public:
        int m_nodeId = 0;
        int m_newParent = 0;
        std::vector<int> m_parents = {};

        std::string m_name = "None";
        std::string m_desc = "Node";

        int m_ruby = 100;
        int m_sapphire = 100;
        int m_emerald = 100;
    };
}