#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>
#include <Framework/Object/Component/UI/UIClickArea.h>
#include "UpgradeTypes.h"
#include "ItemType.h"

#include <unordered_set>
#include <cstdint>
#include "Manager/UpgradeDatabase.h"

namespace game
{
    class UpgradeNodeView;
    
    class UpgradeController :
        public engine::Script<UpgradeController>
    {
        REGISTER_SCRIPT(UpgradeController, Script)

    private:
        struct CostSlot
        {
            engine::GameObject* root = nullptr;
            engine::RectTransform* rt = nullptr;
            engine::UIImage* icon = nullptr;
            engine::UIText* amount = nullptr;

            engine::Vector2 basePos; // 슬롯 원래 위치(초기값)
        };

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    public:
        // 재화 관련
        int GetRuby() const { return m_ruby; }
        int GetSapphire() const { return m_sapphire; }
        int GetEmerald() const { return m_emerald; }

        void SetCurrency(int ruby, int sapphire, int emerald)
        {
            m_ruby = ruby; m_sapphire = sapphire; m_emerald = emerald;
        }

        template<typename Fn>
        void ForEachPurchasedTrue(Fn&& fn) const
        {
            for (const auto& [id, bought] : m_purchased)
                if (bought) fn(id);
        }

        void SetPurchasedFromSet(const std::unordered_set<int>& purchasedSet)
        {
            // BuildNodeTree가 기본 false를 채워줬다는 전제
            for (auto& [id, b] : m_purchased)
                b = false;

            for (int id : purchasedSet)
                m_purchased[id] = true;
        }

        bool CanUpgrade(int nodeId) const;
        bool ApplyUpgrade(int nodeId);

        void SelectNode(int nodeId);
        void ResetSelection();

        void RefreshNodeVisuals();
        void RecomputeUnlocked();
        void RebuildTemperFromPurchased();

    private:
        void LoadDefsFromCsv();
        void ApplyDefsToViews();

        void BuildNodeTree();

        void AutoRegisterNodesFromContent(const std::string& contentRootName);
        
        // Content안에 있는 노드들 모으기
        void AssignNodeIdsFromHierarchy();

        void BindClickArea(const std::string& name, engine::UIClickArea::ClickCallback cb);
        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);
        void BindButton(const std::string& name, engine::UIButton::HoverCallback cb);

        // Category
        void SetCategory(UpgradeCategory c);
        void ApplyCategoryFilter();

        engine::Vector4 GetCategoryColor(UpgradeCategory c) const;

        void UpdateSelectedInfoUI();
        void ClearSelectedInfoUI();

        void BindCostSlots();
        void HideAllCostSlots();
        void LayoutCostSlotsCentered(int visibleCount);
        void UpdateCostUI();
        void SetSlotContent(int visibleIndex, ItemType type, int amount);

    private:
        std::vector<engine::GameObject*> m_nodeObjects;
        std::unordered_map<UpgradeCategory, engine::GameObject*> m_categoryArea;

        std::unordered_map<int, game::UpgradeNodeView*> m_views;

        std::unordered_map<int, bool> m_purchased;
        std::unordered_map<int, bool> m_unlocked;

        UpgradeCategory m_selected = UpgradeCategory::Attack;

        int m_ruby = 1;
        int m_sapphire = 1;
        int m_emerald = 1;

        int m_selectedNodeId = 0;

        CostSlot m_costSlots[3];

        float m_costGapX = 100.0f;

        std::string m_texturePath[3] = {};

        UpgradeDatabase m_db;
        std::string m_dbPath = "Resource/Data/UpgradeNodes.csv";

    private:
        // GameObject
        engine::UISlider* m_scrollBar = nullptr;
        engine::UIText* m_nameText = nullptr;
        engine::UIText* m_descText = nullptr;

        engine::UIText* m_item1Count = nullptr;
        engine::UIText* m_item2Count = nullptr;
        engine::UIText* m_item3Count = nullptr;
    };
}