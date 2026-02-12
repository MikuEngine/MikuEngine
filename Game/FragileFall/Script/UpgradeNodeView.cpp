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

        static engine::Vector4 Lerp(const engine::Vector4& a, const engine::Vector4& b, float t)
        {
            t = std::max(0.0f, std::min(t, 1.0f));
            return a * (1.0f - t) + b * t;
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

        // Display
        ImGui::InputText("Name", &m_name);
        ImGui::InputTextMultiline("Desc", &m_desc);

        // Value Setting
        ImGui::Separator();
        ImGui::Text("Temper Effects");

        static const char* opNames[] = { "Add (합)", "Mul (곱)", "Bool (참/거짓)" };
        static const char* statNames[] = {
            // Attack
            "Atk_Dmg", "Atk_Speed", "Bullet_Range", "Bullet_Size", "Bullet_Speed",

            // Execution (기술/처형)
            "Exe_FragileRegen", "Exe_Range", "Exe_SplashDmg", "Exe_SplashRange", "Exe_DashRegen", "Exe_HpRegen",

            // Vital (체력/생존)
            "Hp_Max", "Hp_RegenOnClear", "Fragile_Max", "Fragile_RegenOnClear", "Fragile_GainRate", "InvincibleTime",

            // Move (이동)
            "Move_Speed", "Dash_Distance", "Dash_Cooldown", "Dash_Invincible",

            // Buff (버프)
            "Buff_MoveSpeedAfterDash", "Buff_AtkDmgAfterDash", "Buff_DurationAfterDash",

            // Special
            "Bullet_Double"
        };

        // (선택) 효과 하나 추가
        if (ImGui::Button("+ Add Effect"))
        {
            TemperEffect e;
            e.op = TemperOp::Add;
            e.stat = StatType::AtkDmg;
            e.value = 0.0f;
            e.b = false;
            m_effects.push_back(e);
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Effects"))
        {
            m_effects.clear();
        }

        // 효과 리스트 편집
        for (int i = 0; i < (int)m_effects.size(); ++i)
        {
            auto& e = m_effects[i];
            ImGui::PushID(i);
            ImGui::Separator();
            ImGui::Text("Effect #%d", i);

            // 연산 방식 선택
            int op = (int)e.op;
            if (ImGui::Combo("Operation", &op, opNames, IM_ARRAYSIZE(opNames)))
                e.op = (TemperOp)op;

            // 스탯 종류 선택 (확장된 목록 사용)
            int st = (int)e.stat;
            if (ImGui::Combo("Target Stat", &st, statNames, IM_ARRAYSIZE(statNames)))
                e.stat = (StatType)st;

            // 값 입력창
            if (e.op == TemperOp::Bool)
            {
                ImGui::Checkbox("Enabled", &e.b);
            }
            else
            {
                // Mul일 경우 사용자가 1.3 같은 수치보다 30(%)으로 입력하는게 편할 수 있습니다.
                if (e.op == TemperOp::Mul)
                {
                    // 필요하다면 여기서 % 단위 UI 노출 로직 추가 가능
                    ImGui::InputFloat("Value (Multiplier)", &e.value);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(ex: 1.1 = +10%%)");
                }
                else
                {
                    ImGui::InputFloat("Value (Amount)", &e.value);
                }
            }

            // 순서 변경(선택)
            if (ImGui::Button("Up") && i > 0)
                std::swap(m_effects[i], m_effects[i - 1]);
            ImGui::SameLine();
            if (ImGui::Button("Down") && i < (int)m_effects.size() - 1)
                std::swap(m_effects[i], m_effects[i + 1]);

            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                m_effects.erase(m_effects.begin() + i);
                ImGui::PopID();
                break; // erase 후 break
            }

            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Text("");

        ImGui::InputInt("Ruby", &m_costRuby);
        ImGui::InputInt("Sapphire", &m_costSapphire);
        ImGui::InputInt("Emerald", &m_costEmerald);
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

        engine::json effects = engine::json::array();
        for (const auto& e : m_effects)
        {
            engine::json ej;
            ej["Op"] = (int)e.op;
            ej["Stat"] = (int)e.stat;
            ej["Value"] = e.value;
            ej["Bool"] = e.b;
            effects.push_back(ej);
        }
        j["Effects"] = effects;

        j["Ruby"] = m_costRuby;
        j["Sapphire"] = m_costSapphire;
        j["Emerald"] = m_costEmerald;
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

        m_effects.clear();

        // Effects가 있으면 그걸 우선 로드
        engine::JsonArrayForEach(j, "Effects",
            [this](const engine::json& ej)
            {
                TemperEffect e;

                int op = (int)e.op;
                int st = (int)e.stat;

                engine::JsonGet(ej, "Op", op);
                engine::JsonGet(ej, "Stat", st);
                engine::JsonGet(ej, "Value", e.value);
                engine::JsonGet(ej, "Bool", e.b);

                e.op = (TemperOp)op;
                e.stat = (StatType)st;

                m_effects.push_back(e);
            });

        engine::JsonGet(j, "Ruby", m_costRuby);
        engine::JsonGet(j, "Sapphire", m_costSapphire);
        engine::JsonGet(j, "Emerald", m_costEmerald);
    }

    void UpgradeNodeView::SetVisualState(NodeState s)
    {
        if (!m_image) return;

        m_image->ClearEffect();
        m_click->SetInteractable(s != NodeState::Disabled);

        float speed, intensity/*, width*/; // warning C4101: 'width' :참조되지 않은 지역 변수입니다.
        engine::Vector4 color = m_baseColor;
        m_image->SetColor(color);

        engine::Vector4 outline = { 0,0,0,0 };
        bool outlineOn = false;

        switch (s)
        {
        case NodeState::Purchased:
            // [강화됨]
            m_image->SetEffect(engine::UIEffectType::PressedSink);
            m_image->SetEffectParam(0, { 0.8f, 0.15f, 0.005f, 0.0f });
            m_image->SetEffectParam(1, { 0.3f, 1.0f, 0.3f, 1.0f });
            outline = { 0.1f, 1.0f, 0.2f, 1.0f }; // 초록 테투리
            outlineOn = true;
            break;

        case NodeState::Selected:
            // [선택됨]
            m_image->SetEffect(engine::UIEffectType::SelectOrbit);
            speed = 2.0f;
            intensity = 0.1f;
            // z:궤도위치, w:궤도강도
            m_image->SetEffectParam(0, { speed, intensity, 0.46f, 3.0f });
            m_image->SetEffectParam(1, { 1.0f, 1.0f, 1.0f, 0.0f });
            m_image->SetEffectParam(2, { 1.0f, 1.0f, 1.0f, 0.0f });

            outline = { 1.0f, 0.9f, 0.1f, 1.0f }; // 황금색
            outlineOn = true;
            break;

        case NodeState::Active:
            m_image->SetEffect(engine::UIEffectType::EnergyFlow);
            speed = 0.8f;
            intensity = 0.5f;
            m_image->SetEffectParam(0, { speed, intensity, 0.0f, 0.0f });
            m_image->SetEffectParam(1, color);

            outline = { 1.0f, 1.0f, 1.0f, 1.0f }; // 흰색 테두리
            outlineOn = true;
            break;

        case NodeState::Disabled:
            // [잠김] 흑백 석화 효과 (색상 전달 불필요)
            m_image->SetEffect(engine::UIEffectType::StoneLock);
            outlineOn = false;
            break;
        }

        m_image->SetOutline(outlineOn, 10.0f, outline);
    }

    void UpgradeNodeView::NormalizeIdsByCategory()
    {
        m_nodeId = MakeNodeId(m_category, m_nodeId % 100);

        for (int& pid : m_parents)
            pid = MakeNodeId(m_category, pid % 100);

        m_newParent = MakeNodeId(m_category, m_newParent % 100);
    }

    void UpgradeNodeView::BuildCostList(std::vector<ItemCost>& out) const
    {
        out.clear();

        if (m_costRuby > 0)
            out.push_back({ ItemType::Ruby, m_costRuby });

        if (m_costSapphire > 0)
            out.push_back({ ItemType::Sapphire, m_costSapphire });

        if (m_costEmerald > 0)
            out.push_back({ ItemType::Emerald, m_costEmerald });
    }
}