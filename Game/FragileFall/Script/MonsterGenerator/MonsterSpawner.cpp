#include "GamePCH.h"
#include "MonsterSpawner.h"
#include "Manager/StageManager.h"

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

    engine::Vector3 MonsterSpawner::GetNextSpawnPosition()
    {
        if (m_allPoints.empty())
            return engine::Vector3(0.0f, 0.0f, 0.0f);

        const size_t numZones = m_zoneStartIndex.empty() ? 0 : m_zoneStartIndex.size() - 1;
        if (numZones == 0)
        {
            engine::Vector3 pos = m_allPoints[0]->GetWorldPosition();
            if (m_spawnRadius > 0.0f)
                pos += engine::Random::InsideUnitCircle() * m_spawnRadius;
            return pos;
        }

        for (size_t z = 0; z < numZones; z++)
        {
            size_t zone = (m_nextZoneIndex + z) % numZones;
            size_t start = m_zoneStartIndex[zone];
            size_t end = m_zoneStartIndex[zone + 1];

            std::vector<size_t> available;
            for (size_t i = start; i < end; i++)
            {
                if (i < m_pointUsed.size() && !m_pointUsed[i])
                    available.push_back(i);
            }
            if (available.empty())
                continue;

            size_t idx = available[engine::Random::Int<size_t>(0, available.size() - 1)];
            m_pointUsed[idx] = true;
            m_nextZoneIndex = (zone + 1) % numZones;
            engine::Vector3 pos = m_allPoints[idx]->GetWorldPosition();
            if (m_spawnRadius > 0.0f)
                pos += engine::Random::InsideUnitCircle() * m_spawnRadius;
            return pos;
        }

        engine::Vector3 pos = m_allPoints[0]->GetWorldPosition();
        if (m_spawnRadius > 0.0f)
            pos += engine::Random::InsideUnitCircle() * m_spawnRadius;
        return pos;
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
        m_spawnedMonsters.push_back(engine::Ptr<engine::GameObject>(go));
        return go;
    }

    void MonsterSpawner::SetStageParams(int targetScore, int minCount, int maxCount, int anchorMonsterID)
    {
        m_targetScore = targetScore;
        m_minCount = minCount;
        m_maxCount = maxCount;
        m_anchorMonsterID = anchorMonsterID;
    }

    void MonsterSpawner::SpawnNow()
    {
        if (!m_partyGenerator.IsDBLoaded() && !m_monsterCsvPath.empty())
            LoadMonsterDB();
        if (!m_partyGenerator.IsDBLoaded())
            return;

        m_nextZoneIndex = 0;
        m_partyGenerator.SetTargetScore(m_targetScore);
        m_partyGenerator.SetCountRange(m_minCount, m_maxCount);
        m_partyGenerator.SetAnchorMonsterID(m_anchorMonsterID);

        std::vector<int> party = m_partyGenerator.GenerateParty();
        SpawnParty(party);
    }

    void MonsterSpawner::Start()
    {
        if (m_managedByStageManager)
        {
            StageManager::Get().OnSpawnerReady(this);
            return;
        }
        SpawnNow();
    }

    void MonsterSpawner::SpawnParty(const std::vector<int>& partyIDs)
    {
        engine::Transform* self = GetTransform();
        if (!self)
            return;

        m_spawnedMonsters.clear();
        m_allPoints.clear();
        m_zoneStartIndex.clear();
        m_zoneStartIndex.push_back(0);

        const std::vector<engine::Transform*>& zones = self->GetChildren();
        for (engine::Transform* zone : zones)
        {
            const std::vector<engine::Transform*>& points = zone->GetChildren();
            for (engine::Transform* pt : points)
                m_allPoints.push_back(pt);
            m_zoneStartIndex.push_back(m_allPoints.size());
        }
        m_pointUsed.assign(m_allPoints.size(), false);

        size_t startZone = 0;
        const size_t numZones = m_zoneStartIndex.size() > 1 ? m_zoneStartIndex.size() - 1 : 0;
        for (size_t z = 0; z < numZones; z++)
        {
            size_t pointCount = m_zoneStartIndex[z + 1] - m_zoneStartIndex[z];
            if (pointCount == 4)
            {
                startZone = z;
                break;
            }
        }
        m_nextZoneIndex = startZone;

        size_t maxSpawn = m_allPoints.size() < 10 ? m_allPoints.size() : 10;
        size_t count = partyIDs.size() < maxSpawn ? partyIDs.size() : maxSpawn;
        for (size_t i = 0; i < count; i++)
        {
            engine::Vector3 pos = GetNextSpawnPosition();
            SpawnOne(partyIDs[i], pos);
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
        ImGui::InputFloat("Spawn Radius", &m_spawnRadius, 0.1f, 1.0f, "%.2f");
    }

    void MonsterSpawner::Save(engine::json& j) const
    {
        Object::Save(j);
        j["MonsterCsvPath"] = m_monsterCsvPath;
        j["TargetScore"] = m_targetScore;
        j["MinCount"] = m_minCount;
        j["MaxCount"] = m_maxCount;
        j["AnchorMonsterID"] = m_anchorMonsterID;
        j["SpawnRadius"] = m_spawnRadius;
    }

    void MonsterSpawner::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "MonsterCsvPath", m_monsterCsvPath);
        engine::JsonGet(j, "TargetScore", m_targetScore);
        engine::JsonGet(j, "MinCount", m_minCount);
        engine::JsonGet(j, "MaxCount", m_maxCount);
        engine::JsonGet(j, "AnchorMonsterID", m_anchorMonsterID);
        engine::JsonGet(j, "SpawnRadius", m_spawnRadius);
    }
}