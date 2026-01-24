#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UpgradeSystem :
        public engine::Script<UpgradeSystem>
    {
        REGISTER_COMPONENT(UpgradeSystem, Script)

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
        int  GetCurrency() const { return m_currency; }
        void SetCurrency(int v) { m_currency = std::max(0, v); RecomputeUnlocked(); }

        bool CanUpgrade(int nodeId) const;
        bool ApplyUpgrade(int nodeId);

    private:
        void BuildDefaultTreeIfEmpty();
        void RecomputeUnlocked();

    private:
        //std::unordered_map<int, UpgradeNode> m_nodes;

        int m_currency = 100;   // 임시 재화
    };
}