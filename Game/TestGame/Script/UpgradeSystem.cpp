#include "GamePCH.h"
#include "UpgradeSystem.h"
#include "UpgradeNodeView.h"

namespace game
{
    void UpgradeSystem::Awake()
    {
        
    }

    void UpgradeSystem::Start()
    {
        BuildDefaultTreeIfEmpty();
    }

    void UpgradeSystem::Update()
    {

    }

    void UpgradeSystem::OnGui()
    {
        static char buf[128] = { 0 };
        ImGui::InputText("Add Node GO Name", buf, sizeof(buf));

        if (ImGui::Button("Add"))
        {
            if (auto* go = engine::GameObject::Find(buf))
                m_nodeObjects.push_back(go);
        }

        ImGui::Separator();
        for (int i = 0; i < (int)m_nodeObjects.size(); ++i)
        {
            ImGui::PushID(i);

            auto* go = m_nodeObjects[i];
            ImGui::Text("%d: %s", i, go ? go->GetName().c_str() : "(null)");

            ImGui::SameLine();
            if (ImGui::Button("X"))
            {
                m_nodeObjects.erase(m_nodeObjects.begin() + i);
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }

        ImGui::Separator();
        if (ImGui::Button("Build From NodeObjects"))
        {
            BuildDefaultTreeIfEmpty();
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

    void UpgradeSystem::Save(engine::json& j) const
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

    void UpgradeSystem::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Ruby", m_ruby);
        engine::JsonGet(j, "Sapphire", m_sapphire);
        engine::JsonGet(j, "Emerald", m_emerald);

        // purchased 초기화
        for (auto& [id, _] : m_purchased)
            m_purchased[id] = false;

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

    bool UpgradeSystem::CanUpgrade(int nodeId) const
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

    bool UpgradeSystem::ApplyUpgrade(int nodeId)
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

    void UpgradeSystem::RefreshNodeVisuals()
    {
        for (auto& [id, view] : m_views)
        {
            if (!view) continue;
            
            const bool purchased = (m_purchased.find(id) != m_purchased.end()) ? m_purchased[id] : false;
            const bool unlocked = (m_unlocked.find(id) != m_unlocked.end()) ? m_unlocked[id] : false;

            view->SetVisualState(unlocked, purchased);
        }
    }

    void UpgradeSystem::BuildDefaultTreeIfEmpty()
    {
        m_views.clear();

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

    void UpgradeSystem::RecomputeUnlocked()
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

            bool ok = true;
            for (int pid : view->m_parents)
            {
                auto it = m_purchased.find(pid);
                if (it == m_purchased.end() || !it->second)
                {
                    ok = false;
                    break;
                }
            }

            m_unlocked[id] = ok;
        }
    }
}