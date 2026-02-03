#include "GamePCH.h"
#include "MonsterSpawner.h"

#include <Framework/Asset/Prefab.h>

namespace game
{
    bool MonsterSpawner::LoadMonsterDB()
    {
        if (m_monsterCsvPath.empty())
        {
            LOG_ERROR("[MonsterSpawner] Monster CSV path is empty");
            return false;
        }
        return m_partyGenerator.LoadMonsterDB(m_monsterCsvPath);
    }

    std::string MonsterSpawner::GetPrefabNameFromID(int monsterID) const
    {
        const MonsterData* p = m_partyGenerator.FindMonsterByID(monsterID);
        return p ? p->monsterName : std::string();
    }

    engine::Vector3 MonsterSpawner::GetRandomSpawnPosition() const
    {
        engine::Transform* self = GetTransform();
        if (!self)
        {
            return engine::Vector3(0.0f, 0.0f, 0.0f);
        }

        const std::vector<engine::Transform*>& children = self->GetChildren();
        if (children.empty())
        {
            return self->GetWorldPosition();
        }

        size_t idx = engine::Random::Int<size_t>(0, children.size() - 1);
        return children[idx]->GetWorldPosition();
    }

    engine::GameObject* MonsterSpawner::SpawnOne(int monsterID, const engine::Vector3& position)
    {
        std::string prefabName = GetPrefabNameFromID(monsterID);
        if (prefabName.empty())
        {
            LOG_ERROR("[MonsterSpawner] No prefab name for monster ID {}", monsterID);
            return nullptr;
        }

        engine::GameObject* go = engine::Prefab::Instantiate(prefabName);
        if (!go)
        {
            LOG_ERROR("[MonsterSpawner] Failed to instantiate prefab '{}'", prefabName);
            return nullptr;
        }

        go->GetTransform()->SetLocalPosition(position);
        return go;
    }

    void MonsterSpawner::Start()
    {
        if (!m_partyGenerator.IsDBLoaded() && !m_monsterCsvPath.empty())
        {
            LoadMonsterDB();
        }
        if (!m_partyGenerator.IsDBLoaded())
            return;

        m_partyGenerator.SetTargetScore(m_targetScore);
        m_partyGenerator.SetCountRange(m_minCount, m_maxCount);
        m_partyGenerator.SetAnchorMonsterID(m_anchorMonsterID);

        std::vector<int> party = m_partyGenerator.GenerateParty();
        SpawnParty(party);
    }

    void MonsterSpawner::SpawnParty(const std::vector<int>& partyIDs)
    {
        for (int monsterID : partyIDs)
        {
            engine::Vector3 pos = GetRandomSpawnPosition();
            SpawnOne(monsterID, pos);
        }
    }

    void MonsterSpawner::OnGui()
    {
        ImGui::InputText("Monster CSV Path", &m_monsterCsvPath);
        if (ImGui::Button("Load Monster DB"))
        {
            LoadMonsterDB();
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(m_partyGenerator.IsDBLoaded() ? "OK" : "Not loaded");

        ImGui::Separator();
        ImGui::Text("Scene params (used in Start)");
        ImGui::InputInt("Target Score", &m_targetScore);
        ImGui::InputInt("Min Count", &m_minCount);
        ImGui::InputInt("Max Count", &m_maxCount);
        ImGui::InputInt("Anchor Monster ID", &m_anchorMonsterID);
    }

    void MonsterSpawner::Save(engine::json& j) const
    {
        Object::Save(j);
        j["MonsterCsvPath"] = m_monsterCsvPath;
        j["TargetScore"] = m_targetScore;
        j["MinCount"] = m_minCount;
        j["MaxCount"] = m_maxCount;
        j["AnchorMonsterID"] = m_anchorMonsterID;
    }

    void MonsterSpawner::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "MonsterCsvPath", m_monsterCsvPath);
        engine::JsonGet(j, "TargetScore", m_targetScore);
        engine::JsonGet(j, "MinCount", m_minCount);
        engine::JsonGet(j, "MaxCount", m_maxCount);
        engine::JsonGet(j, "AnchorMonsterID", m_anchorMonsterID);
    }
}