#include "GamePCH.h"
#include "UpgradeController.h"
#include "UpgradeNodeView.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UISlider.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>

namespace game
{
    namespace
    {
        // contentRoot 아래를 재귀 순회하면서 UpgradeNodeView가 달린 GO를 모두 수집
        static void CollectUpgradeNodesRecursive(engine::Transform* t, std::vector<engine::GameObject*>& out)
        {
            if (!t) return;

            engine::GameObject* go = t->GetGameObject();
            if (go && go->GetComponent<game::UpgradeNodeView>())
                out.push_back(go);

            for (engine::Transform* c : t->GetChildren())
                CollectUpgradeNodesRecursive(c, out);
        }

        static int MakeNodeId(UpgradeCategory cat, int localIndex)
        {
            // localIndex: 0,1,2...
            return (int)cat + localIndex;
        }

        static UpgradeCategory GetCategoryFromId(int id)
        {
            const int base = (id / 100) * 100;
            return (UpgradeCategory)base;
        }

        static int GetLocalIndexFromId(int id)
        {
            return (id % 100);
        }
    }

    void UpgradeController::Awake()
    {
        BindButton("Btn_Attack", [self = engine::Ptr<UpgradeController>(this)]() {if (self) self->SetCategory(UpgradeCategory::Attack); });
        BindButton("Btn_Skill", [self = engine::Ptr<UpgradeController>(this)]() {if (self) self->SetCategory(UpgradeCategory::Skill); });
        BindButton("Btn_Survive", [self = engine::Ptr<UpgradeController>(this)]() {if (self) self->SetCategory(UpgradeCategory::Life); });
        BindButton("Btn_Move", [self = engine::Ptr<UpgradeController>(this)]() {if (self) self->SetCategory(UpgradeCategory::Move);});
        
        BindButton("Btn_Upgrade", [self = engine::Ptr<UpgradeController>(this)]() {if (!self)return; if (self->m_selectedNodeId == 0) return; self->ApplyUpgrade(self->m_selectedNodeId); });
    }

    void UpgradeController::Start()
    {
        auto* go = engine::GameObject::Find("Scrollbar");
        if (go)
            m_scrollBar = go->GetComponent<engine::UISlider>();

        auto* nameGO = engine::GameObject::Find("Text_UpgradeName");
        if (nameGO) m_nameText = nameGO->GetComponent<engine::UIText>();

        auto* descGO = engine::GameObject::Find("Text_UpgradeDesc");
        if (descGO) m_descText = descGO->GetComponent<engine::UIText>();

        auto* item1GO = engine::GameObject::Find("Text_Item1Count");
        if (item1GO) m_item1Count = item1GO->GetComponent<engine::UIText>();

        auto* item2GO = engine::GameObject::Find("Text_Item2Count");
        if (item2GO) m_item2Count = item2GO->GetComponent<engine::UIText>();

        auto* item3GO = engine::GameObject::Find("Text_Item3Count");
        if (item3GO) m_item3Count = item3GO->GetComponent<engine::UIText>();

        ClearSelectedInfoUI();

        AutoRegisterNodesFromContent("Content");

        BuildNodeTree();
        ApplyCategoryFilter();
    }

    void UpgradeController::Update()
    {
        m_item1Count->SetText(std::to_string(m_ruby));
        m_item2Count->SetText(std::to_string(m_sapphire));
        m_item3Count->SetText(std::to_string(m_emerald));
    }

    void UpgradeController::OnGui()
    {
        if (ImGui::InputInt3("Currency", &m_ruby))
        {
            m_ruby = std::max(0, m_ruby);
            m_sapphire = std::max(0, m_sapphire);
            m_emerald = std::max(0, m_emerald);

            RecomputeUnlocked();
            RefreshNodeVisuals();
        }

        ImGui::Text("Wallet: Ruby=%d Sapphire=%d Emerald=%d", m_ruby, m_sapphire, m_emerald);
    }

    void UpgradeController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Ruby"] = m_ruby;
        j["Sapphire"] = m_sapphire;
        j["Emerald"] = m_emerald;

        engine::json purchased = engine::json::array();
        for (const auto& [id, isBought] : m_purchased)
        {
            if (isBought)
                purchased.push_back(id);
        }
        j["Purchased"] = purchased;
    }

    void UpgradeController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Ruby", m_ruby);
        engine::JsonGet(j, "Sapphire", m_sapphire);
        engine::JsonGet(j, "Emerald", m_emerald);

        m_purchased.clear();
        m_unlocked.clear();
        m_views.clear();

        // 노드 오브젝트 로드
        AutoRegisterNodesFromContent("Content");

        BuildNodeTree();

        // 저장된 구매 목록 반영
        engine::JsonArrayForEach(j, "Purchased",
            [this](const engine::json& v)
            {
                const int id = v.get<int>();
                m_purchased[id] = true;
            });

        RecomputeUnlocked();
        RefreshNodeVisuals();
    }

    bool UpgradeController::CanUpgrade(int nodeId) const
    {
        auto itV = m_views.find(nodeId);
        if (itV == m_views.end() || !itV->second)
            return false;

        auto itP = m_purchased.find(nodeId);
        if (itP != m_purchased.end() && itP->second)
            return false;

        auto itU = m_unlocked.find(nodeId);
        if (itU == m_unlocked.end() || !itU->second)
            return false;

        const auto* view = itV->second;

        if (m_ruby < view->m_ruby) return false;
        if (m_sapphire < view->m_sapphire) return false;
        if (m_emerald < view->m_emerald) return false;

        return true;
    }

    bool UpgradeController::ApplyUpgrade(int nodeId)
    {
        if (!CanUpgrade(nodeId))
            return false;

        auto* view = m_views[nodeId];

        m_ruby -= view->m_ruby;
        m_sapphire -= view->m_sapphire;
        m_emerald -= view->m_emerald;

        m_purchased[nodeId] = true;

        RecomputeUnlocked();
        RefreshNodeVisuals();
        return true;
    }

    void UpgradeController::SelectNode(int nodeId)
    {
        if (m_views.find(nodeId) == m_views.end())
        {
            m_selectedNodeId = 0;
            RefreshNodeVisuals();
            return;
        }

        m_selectedNodeId = nodeId;

        RefreshNodeVisuals();
        UpdateSelectedInfoUI();
    }

    void UpgradeController::RefreshNodeVisuals()
    {
        for (auto& [id, view] : m_views)
        {
            if (!view) continue;
            
            const bool purchased = (m_purchased.find(id) != m_purchased.end()) ? m_purchased[id] : false;
            const bool unlocked = (m_unlocked.find(id) != m_unlocked.end()) ? m_unlocked[id] : false;
            const bool selected = (id == m_selectedNodeId);

            UpgradeNodeView::NodeState s;

            if (purchased) s = UpgradeNodeView::NodeState::Purchased;
            else if (!unlocked) s = UpgradeNodeView::NodeState::Disabled;
            else if (selected) s = UpgradeNodeView::NodeState::Selected;
            else s = UpgradeNodeView::NodeState::Active;

            view->SetVisualState(s);
        }
    }

    void UpgradeController::BuildNodeTree()
    {
        m_views.clear();

        LOG_PRINT("[Upgrade] Collected={}", (int)m_nodeObjects.size());

        for (auto* go : m_nodeObjects)
        {
            if (!go) continue;

            auto* view = go->GetComponent<game::UpgradeNodeView>();
            if (!view) continue;

            const int id = view->m_nodeId;
            if (id == 0) continue;

            // 중복 id 방지
            if (m_views.find(id) != m_views.end())
                continue;

            m_views[id] = view;

            // 상태 기본값 등록
            if (m_purchased.find(id) == m_purchased.end())
                m_purchased[id] = false;
        }

        RecomputeUnlocked();
        RefreshNodeVisuals();
    }

    void UpgradeController::RecomputeUnlocked()
    {
        m_unlocked.clear();

        for (auto& [id, view] : m_views)
        {
            if (!view) continue;

            if (m_purchased[id])
            {
                m_unlocked[id] = true;
                continue;
            }

            if (view->m_parents.empty())
            {
                m_unlocked[id] = true;
                continue;
            }

            // 선행노드를 모두 구매해야 조건 성립
            //bool ok = true;
            //for (int pid : view->m_parents)
            //{
            //    auto it = m_purchased.find(pid);
            //    if (it == m_purchased.end() || !it->second)
            //    {
            //        ok = false;
            //        break;
            //    }
            //}

            bool anyBought = false;
            for (int pid : view->m_parents)
            {
                auto it = m_purchased.find(pid);
                if (it != m_purchased.end() && it->second)
                {
                    anyBought = true;
                    break;
                }
            }

            m_unlocked[id] = anyBought;
        }
    }

    void UpgradeController::AutoRegisterNodesFromContent(const std::string& contentRootName)
    {
        m_nodeObjects.clear();

        auto* rootGO = engine::GameObject::Find(contentRootName);
        if (!rootGO) return;

        auto* rootT = rootGO->GetTransform();
        if (!rootT) return;

        CollectUpgradeNodesRecursive(rootT, m_nodeObjects);
    }

    void UpgradeController::BindClickArea(const std::string& name, engine::UIClickArea::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* ca = go->GetComponent<engine::UIClickArea>();
        if (!ca) return;

        ca->AddOnClick(std::move(cb));
    }

    void UpgradeController::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick(std::move(cb));
    }

    void UpgradeController::SetCategory(UpgradeCategory c)
    {
        if (m_selected == c) return;
        m_selected = c;
        
        m_selectedNodeId = 0;
        ClearSelectedInfoUI();
        RefreshNodeVisuals();

        ApplyCategoryFilter();
        RefreshNodeVisuals();

        if (m_scrollBar)
            m_scrollBar->SetValue(0.0f, true);
    }

    void UpgradeController::ApplyCategoryFilter()
    {
        LOG_PRINT("[Filter] selected={}", (int)m_selected);

        for (auto* go : m_nodeObjects)
        {
            if (!go) continue;
            auto* view = go->GetComponent<UpgradeNodeView>();
            if (!view) continue;

            LOG_PRINT("[Filter] node={} cat={} ", view->m_nodeId, (int)view->m_category);

            const bool show = (view->m_category == m_selected);
            go->SetActive(show);
        }
    }
    engine::Vector4 UpgradeController::GetCategoryColor(UpgradeCategory c) const
    {
        for (auto& [id, view] : m_views)
        {
            if (!view) continue;
            if (view->m_category != c) continue;

            return view->m_nodeColor; // 첫 노드 색
        }

        return { 1,1,1,1 }; // fallback
    }

    void UpgradeController::UpdateSelectedInfoUI()
    {
        if (m_selectedNodeId == 0)
        {
            ClearSelectedInfoUI();
            return;
        }

        auto it = m_views.find(m_selectedNodeId);

        UpgradeNodeView* view = it->second;

        if (it == m_views.end() || !it->second)
        {
            ClearSelectedInfoUI();
            return;
        }

        if (m_nameText) m_nameText->SetText(view->m_name);
        if (m_descText) m_descText->SetText(view->m_desc);
    }

    void UpgradeController::ClearSelectedInfoUI()
    {
        if (m_nameText) m_nameText->SetText("");
        if (m_descText) m_descText->SetText("");
    }
}