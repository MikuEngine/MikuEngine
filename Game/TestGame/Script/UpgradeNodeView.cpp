#include "GamePCH.h"
#include "UpgradeNodeView.h"

namespace game
{
    void UpgradeNodeView::Awake()
    {

    }

    void UpgradeNodeView::OnGui()
    {
        ImGui::InputInt("NodeId", &m_nodeId);
        


        ImGui::Text("Parents");
        for (int i = 0; i < (int)m_parents.size(); ++i)
        {
            ImGui::PushID(i);

            int v = m_parents[i];
            if (ImGui::InputInt("##Parent", &v))
            {
                // 자기 자신 금지
                if (v != m_nodeId)
                {
                    // 중복 방지
                    bool dup = false;
                    for (int k = 0; k < (int)m_parents.size(); ++k)
                    {
                        if (k != i && m_parents[k] == v)
                        {
                            dup = true;
                            break;
                        }
                    }

                    if (!dup)
                        m_parents[i] = v;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("X"))
            {
                m_parents.erase(m_parents.begin() + i);
                ImGui::PopID();
                break; // erase 이후 바로 탈출
            }

            ImGui::PopID();
        }

        ImGui::Separator();

        // 새 부모 추가
        static int s_newParent = 0;
        ImGui::InputInt("New Parent", &s_newParent);

        bool canAdd = (s_newParent != m_nodeId);
        for (int pid : m_parents)
        {
            if (pid == s_newParent)
            {
                canAdd = false;
                break;
            }
        }

        if (!canAdd) ImGui::BeginDisabled(true);
        if (ImGui::Button("+ Add Parent"))
        {
            m_parents.push_back(s_newParent);
            s_newParent = 0;
        }
        if (!canAdd) ImGui::EndDisabled();






        ImGui::InputText("Name", &m_name);
        ImGui::InputTextMultiline("Desc", &m_desc);

        ImGui::InputInt("Ruby", &m_ruby);
        ImGui::InputInt("Sapphire", &m_sapphire);
        ImGui::InputInt("Emerald", &m_emerald);
    }

    void UpgradeNodeView::Save(engine::json& j) const
    {
        Object::Save(j);

        j["NodeId"] = m_nodeId;

        engine::json parents = engine::json::array();
        for (int pid : m_parents)
            parents.push_back(pid);
        j["Parent"] = parents;

        j["Name"] = m_name;
        j["Desc"] = m_desc;

        j["Ruby"] = m_ruby;
        j["Sapphire"] = m_sapphire;
        j["Emerald"] = m_emerald;
    }

    void UpgradeNodeView::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "NodeId", m_nodeId);

        m_parents.clear();
        engine::JsonArrayForEach(j, "Parent",
            [this](const engine::json& v)
            {
                m_parents.push_back(v.get<int>());
            });

        engine::JsonGet(j, "Name", m_name);
        engine::JsonGet(j, "Desc", m_desc);
        
        engine::JsonGet(j, "Ruby", m_ruby);
        engine::JsonGet(j, "Sapphire", m_sapphire);
        engine::JsonGet(j, "Emerald", m_emerald);
    }
}