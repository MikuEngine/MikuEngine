#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIClickArea.h>

#include "UpgradeTypes.h"
#include "ItemType.h"
#include "Script/CharacterScript/Player/StatId.h"

namespace game
{
    // States
    enum class TemperOp { Add, Mul, Bool };

    // 버프 필드(파라미터)
    enum class BuffField
    {
        Duration,     // float
        Bonus,        // float
    };

    struct TemperEffect
    {
        TemperOp op = TemperOp::Add;

        enum class Kind { Stat, Buff, Special } kind;

        StatType stat = StatType::AtkDmg;

        BuffId buff = BuffId::Dash_MoveSpeed;
        BuffField field = BuffField::Bonus;

        float value = 0.0f;
        bool b = false;
    };

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
        std::vector<TemperEffect> m_effects;

    private:
        engine::Vector4 m_baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        engine::UIImage* m_image = nullptr;
        engine::UIClickArea* m_click = nullptr;
    };
}