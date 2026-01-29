#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>
#include <Framework/Object/Component/UI/UIClickArea.h>
#include "UpgradeTypes.h"

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
        void SelectNode(int nodeId);

        void RefreshNodeVisuals();

    private:
        void BuildNodeTree();
        void RecomputeUnlocked();
        void AutoRegisterNodesFromContent(const std::string& contentRootName);
        
        void BindClickArea(const std::string& name, engine::UIClickArea::ClickCallback cb);
        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);

        // Category
        void SetCategory(UpgradeCategory c);
        void ApplyCategoryFilter();

        engine::Vector4 GetCategoryColor(UpgradeCategory c) const;

        void UpdateSelectedInfoUI();
        void ClearSelectedInfoUI();

    private:
        std::vector<engine::GameObject*> m_nodeObjects;
        std::unordered_map<UpgradeCategory, engine::GameObject*> m_categoryArea;

        std::unordered_map<int, game::UpgradeNodeView*> m_views;

        std::unordered_map<int, bool> m_purchased;
        std::unordered_map<int, bool> m_unlocked;

        UpgradeCategory m_selected = UpgradeCategory::Attack;

        int m_ruby = 100;
        int m_sapphire = 100;
        int m_emerald = 100;

        int m_selectedNodeId = 0;

    private:
        // GameObject
        engine::UISlider* m_scrollBar = nullptr;
        engine::UIText* m_nameText = nullptr;
        engine::UIText* m_descText = nullptr;
    };
}