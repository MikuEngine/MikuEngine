#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIClickArea.h>

#include "UpgradeTypes.h"

namespace game
{
    class UpgradeNodeView :
        public engine::Script<UpgradeNodeView>
    {
        REGISTER_SCRIPT(UpgradeNodeView, Script)

    public:
        enum class NodeState
        {
            Purchased,
            Selected,
            Active,
            Disabled
        };

    public:
        void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

        void SetVisualState(NodeState s);

        void NormalizeIdsByCategory();

    public:
        UpgradeCategory m_category = UpgradeCategory::Attack;

        int m_nodeId = 0;
        int m_newParent = 0;
        std::vector<int> m_parents = {};

        std::string m_name = "None";
        std::string m_desc = "Node";

        int m_ruby = 100;
        int m_sapphire = 100;
        int m_emerald = 100;

        engine::Vector4 m_nodeColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    private:
        engine::Vector4 m_baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        engine::UIImage* m_image = nullptr;
        engine::UIClickArea* m_click = nullptr;
    };
}