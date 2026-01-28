#include "GamePCH.h"
#include "UpgradeController.h"
#include "UpgradeNodeView.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UISlider.h>

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
    }

    void UpgradeController::Awake()
    {
        BindButton("Btn_Attack", [this] { SetCategory(UpgradeCategory::Attack); });
        BindButton("Btn_Skill", [this] { SetCategory(UpgradeCategory::Skill); });
        BindButton("Btn_Life", [this] { SetCategory(UpgradeCategory::Life); });
        BindButton("Btn_Move", [this] { SetCategory(UpgradeCategory::Move); });
    }

    void UpgradeController::Start()
    {
        auto* go = engine::GameObject::Find("Scrollbar");
        if (go)
            m_scrollBar = go->GetComponent<engine::UISlider>();

        AutoRegisterNodesFromContent("Content");

        BuildNodeTree();
        ApplyCategoryFilter();
    }

    void UpgradeController::Update()
    {

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

        for (auto& [id, view] : m_views)
        {
            ImGui::PushID(id);
            ImGui::Text("Node %d : %s", id, view ? view->m_name.c_str() : "(null)");
            ImGui::SameLine();
            if (ImGui::Button("Try Upgrade"))
            {
                ApplyUpgrade(id);
            }
            ImGui::PopID();
        }
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

    void UpgradeController::RefreshNodeVisuals()
    {
        for (auto& [id, view] : m_views)
        {
            if (!view) continue;
            
            const bool purchased = (m_purchased.find(id) != m_purchased.end()) ? m_purchased[id] : false;
            const bool unlocked = (m_unlocked.find(id) != m_unlocked.end()) ? m_unlocked[id] : false;

            view->SetVisualState(unlocked, purchased);
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
        ApplyCategoryFilter();

        if (m_scrollBar)
            m_scrollBar->SetValue(0.0f, true);
    }

    void UpgradeController::ApplyCategoryFilter()
    {
        for (auto* go : m_nodeObjects)
        {
            if (!go) continue;
            auto* view = go->GetComponent<UpgradeNodeView>();
            if (!view) continue;

            const bool show = (view->m_category == m_selected);
            go->SetActive(show);
        }
    }
}