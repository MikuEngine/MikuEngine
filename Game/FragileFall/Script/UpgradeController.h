#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UpgradeNodeView;
    
    class UpgradeController :
        public engine::Script<UpgradeController>
    {
        REGISTER_SCRIPT(UpgradeController, Script)

    public:

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    public:
        // 재화
        //int  GetCurrency() const { return m_currency; }
        //void SetCurrency(int v) { m_currency = std::max(0, v); RecomputeUnlocked(); }

        bool CanUpgrade(int nodeId) const;
        bool ApplyUpgrade(int nodeId);

        void RefreshNodeVisuals();

    private:
        void BuildDefaultTreeIfEmpty();
        void RecomputeUnlocked();
        
    private:
        //std::unordered_map<int, UpgradeNode> m_nodes;
        std::vector<engine::GameObject*> m_nodeObjects;

        std::unordered_map<int, game::UpgradeNodeView*> m_views;

        std::unordered_map<int, bool> m_purchased;
        std::unordered_map<int, bool> m_unlocked;

        int m_ruby = 100;
        int m_sapphire = 100;
        int m_emerald = 100;
    };
}