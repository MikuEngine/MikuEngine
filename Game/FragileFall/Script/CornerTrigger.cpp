#include "GamePCH.h"
#include "CornerTrigger.h"


namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // GUI
    // ═══════════════════════════════════════════════════════════════
    void CornerTrigger::OnGui()
    {
        if (ImGui::CollapsingHeader("Corner Trigger", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Blocked Directions (Corner)");
            ImGui::Separator();
            
            // 막힌 방향 1
            const char* dirOptions[] = { "None", "+X (Right)", "-X (Left)", "+Z (Up)", "-Z (Down)" };
            int dir1Index = static_cast<int>(m_blockedDir1);
            if (ImGui::Combo("Blocked Dir 1", &dir1Index, dirOptions, IM_ARRAYSIZE(dirOptions)))
            {
                m_blockedDir1 = static_cast<BlockedDirection>(dir1Index);
            }
            
            // 막힌 방향 2
            int dir2Index = static_cast<int>(m_blockedDir2);
            if (ImGui::Combo("Blocked Dir 2", &dir2Index, dirOptions, IM_ARRAYSIZE(dirOptions)))
            {
                m_blockedDir2 = static_cast<BlockedDirection>(dir2Index);
            }
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                "Example: TopRight corner = +X, +Z blocked");
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════
    void CornerTrigger::Save(engine::json& j) const
    {
        // 부모 클래스 Save 호출 (Type, Active 등 저장)
        Object::Save(j);
        
        j["blockedDir1"] = static_cast<int>(m_blockedDir1);
        j["blockedDir2"] = static_cast<int>(m_blockedDir2);
    }

    void CornerTrigger::Load(const engine::json& j)
    {
        // 부모 클래스 Load 호출
        Object::Load(j);
        
        if (j.contains("blockedDir1"))
            m_blockedDir1 = static_cast<BlockedDirection>(j["blockedDir1"].get<int>());
        if (j.contains("blockedDir2"))
            m_blockedDir2 = static_cast<BlockedDirection>(j["blockedDir2"].get<int>());
    }
}
