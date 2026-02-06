#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIClickArea.h>

#include "UpgradeTypes.h"
#include "ItemType.h"

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

        void BuildCostList(std::vector<ItemCost>& out) const;

    public:
        UpgradeCategory m_category = UpgradeCategory::Attack;

        int m_nodeId = 0;
        int m_newParent = 0;
        std::vector<int> m_parents = {};

        std::string m_name = "None";
        std::string m_desc = "Node";

        int m_costRuby = 100;
        int m_costSapphire = 100;
        int m_costEmerald = 100;

        engine::Vector4 m_nodeColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    public:
        // States
        enum class TemperOp { Add, Mul, Bool };
        enum class TemperStat
        {
            // Attack
            AtkDmg,
            AtkSpeed,
            BulletRange,
            BulletSizeScale,
            BulletSpeed,
            BulletDouble,




        };

        struct TemperEffect
        {
            TemperOp op = TemperOp::Add;
            TemperStat stat = TemperStat::AtkDmg;
            float value;
            bool b;
        };

        std::vector<TemperEffect> m_effects;

    private:
        engine::Vector4 m_baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        engine::UIImage* m_image = nullptr;
        engine::UIClickArea* m_click = nullptr;
    };
}