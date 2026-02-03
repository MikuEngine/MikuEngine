#include "GamePCH.h"
#include "MonsterSpawner.h"
#include <Common/Debug/Debug.h>
#include <Common/Math/MathUtility.h>
#include <Framework/Asset/Prefab.h>
#include <Framework/Object/GameObject/GameObject.h>

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
            return engine::Vector3(0.0f, 0.0f, 0.0f);

        const std::vector<engine::Transform*>& children = self->GetChildren();
        if (children.empty())
            return self->GetWorldPosition();

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

    void MonsterSpawner::OnGui()
    {
        ImGui::InputText("Monster CSV Path", &m_monsterCsvPath);
        if (ImGui::Button("Load Monster DB"))
        {
            LoadMonsterDB();
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(m_partyGenerator.IsDBLoaded() ? "OK" : "Not loaded");
    }

    void MonsterSpawner::Save(engine::json& j) const
    {
        Object::Save(j);
        j["MonsterCsvPath"] = m_monsterCsvPath;
    }

    void MonsterSpawner::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "MonsterCsvPath", m_monsterCsvPath);
    }
}