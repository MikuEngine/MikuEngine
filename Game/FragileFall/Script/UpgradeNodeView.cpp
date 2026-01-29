#include "GamePCH.h"
#include "UpgradeNodeView.h"
#include "UpgradeController.h"

namespace game
{
    namespace
    {
        static int GetLocal(int id) { return id % 100; }
        static int MakeNodeId(UpgradeCategory cat, int local)
        {
            local = std::clamp(local, 0, 99);
            return (int)cat + local; // cat = 100/200/...
        }
    }

    void UpgradeNodeView::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_click = go->GetComponent<engine::UIClickArea>();
        if (!m_click) return;

        auto* sysGo = engine::GameObject::Find("UpgradeController");
        if (!sysGo) return;

        auto* sys = sysGo->GetComponent<game::UpgradeController>();
        if (!sys) return;

        m_click->AddOnClick([this, sys](int mouseButton)
            {
                if (mouseButton != 0) return;

                if (sys)
                    sys->SelectNode(m_nodeId);
            });

        m_image = go->GetComponent<engine::UIImage>();
        if (!m_image) return;

        NormalizeIdsByCategory();
    }

    void UpgradeNodeView::OnGui()
    {
        static const char* kCats[] = { "Attack", "Skill", "Survive", "Move" };

        auto ToIndex = [](UpgradeCategory cat) -> int
            {
                switch (cat)
                {
                case UpgradeCategory::Attack: return 0;
                case UpgradeCategory::Skill:  return 1;
                case UpgradeCategory::Life:   return 2;
                case UpgradeCategory::Move:   return 3;
                default: return 0;
                }
            };

        auto FromIndex = [](int idx) -> UpgradeCategory
            {
                switch (idx)
                {
                case 0: return UpgradeCategory::Attack;
                case 1: return UpgradeCategory::Skill;
                case 2: return UpgradeCategory::Life;
                case 3: return UpgradeCategory::Move;
                default: return UpgradeCategory::Attack;
                }
            };

        int c = ToIndex(m_category);
        if (ImGui::Combo("Category", &c, kCats, IM_ARRAYSIZE(kCats)))
        {
            m_category = FromIndex(c);
            NormalizeIdsByCategory();
        }

        int local = GetLocal(m_nodeId);

        if (ImGui::InputInt("Node LocalId (0~99)", &local))
        {
            m_nodeId = MakeNodeId(m_category, local);
            NormalizeIdsByCategory(); // 부모도 함께 보정할 거면 유지
        }


        ImGui::Text("Parents");
        for (int i = 0; i < (int)m_parents.size(); ++i)
        {
            ImGui::PushID(i);

            int vLocal = GetLocal(m_parents[i]);

            if (ImGui::InputInt("##Parent", &vLocal))
            {
                int v = MakeNodeId(m_category, vLocal);

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


        int newLocal = m_newParent % 100;               // 표시/입력은 local
        if (ImGui::InputInt("New Parent (0~99)", &newLocal))
        {
            newLocal = std::clamp(newLocal, 0, 99);
            m_newParent = MakeNodeId(m_category, newLocal); // 내부는 full id로 유지
        }

        bool canAdd = (m_newParent != m_nodeId);
        for (int pid : m_parents)
        {
            if (pid == m_newParent) { canAdd = false; break; }
        }

        if (!canAdd) ImGui::BeginDisabled(true);
        if (ImGui::Button("+ Add Parent"))
        {
            m_parents.push_back(m_newParent);
            m_newParent = MakeNodeId(m_category, 0);
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

        int idx = 0;
        switch (m_category)
        {
        case UpgradeCategory::Attack: idx = 0; break;
        case UpgradeCategory::Skill:  idx = 1; break;
        case UpgradeCategory::Life:   idx = 2; break;
        case UpgradeCategory::Move:   idx = 3; break;
        }
        j["Category"] = idx;
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

        int idx = 0;
        engine::JsonGet(j, "Category", idx);
        idx = std::clamp(idx, 0, 3);

        switch (idx)
        {
        case 0: m_category = UpgradeCategory::Attack; break;
        case 1: m_category = UpgradeCategory::Skill;  break;
        case 2: m_category = UpgradeCategory::Life;   break;
        case 3: m_category = UpgradeCategory::Move;   break;
        }

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

    void UpgradeNodeView::SetVisualState(NodeState s)
    {
        if (!m_image) return;

        engine::Vector4 fill = m_baseColor;
        engine::Vector4 outline = { 0,0,0,0 };
        bool outlineOn = false;

        m_click->SetInteractable(s != NodeState::Disabled);

        switch (s)
        {
        case NodeState::Purchased:
            fill = m_baseColor;
            outline = { 1.0f, 238 / 255.0f, 24 / 255.0f, 1.0f }; // 노랑
            outlineOn = true;
            break;

        case NodeState::Selected:
            fill = m_baseColor;
            outline = { 0.0f, 180 / 255.0f, 230 / 255.0f, 1.0f }; // 파랑
            outlineOn = true;
            break;

        case NodeState::Active:
            fill = m_baseColor;
            outline = { 0.4f, 0.4f, 0.4f, 1.0f }; // 회색 테두리
            outlineOn = true;
            break;

        case NodeState::Disabled:
            fill = { 0.4f, 0.4f, 0.4f, 1.0f };    // 전체 회색
            outlineOn = false;
            break;
        }

        m_image->SetColor(fill);
        m_image->SetOutline(outlineOn, 5.0f, outline);
    }

    void UpgradeNodeView::NormalizeIdsByCategory()
    {
        m_nodeId = MakeNodeId(m_category, m_nodeId % 100);

        for (int& pid : m_parents)
            pid = MakeNodeId(m_category, pid % 100);

        m_newParent = std::clamp(m_newParent, 0, 99);
    }
}