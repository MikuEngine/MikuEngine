#include "GamePCH.h"
#include "StageManager.h"

#include <Engine/Framework/Scene/SceneManager.h>
#include <Engine/Framework/Scene/Scene.h>
#include <Engine/Framework/Asset/Prefab.h>
#include <Engine/Framework/Object/GameObject/GameObject.h>
#include <Engine/Framework/Object/Component/Transform.h>
#include <Engine/Framework/Object/Component/BoxCollider.h>
#include <Engine/Framework/Object/Component/Light/Light.h>
#include <Engine/Common/Utility/CSVReader.h>

#include "LoadingScreenDrawer.h"
#include "UpgradeProgressManager.h"
#include "Script/MonsterGenerator/MonsterSpawner.h"
#include "Script/DoorTriggerScript.h"

namespace game
{
    namespace
    {
        constexpr const char* g_stageCsvPath = "Resource/Data/Stages.csv";
        constexpr const char* g_defaultMapEnvPrefab = "";  // 비어 있으면 맵 환경 프리팹 미로드
        const std::array<const char*, 3> g_spawnerRootNames{ "StageRoom_A", "StageRoom_B", "StageRoom_C" };

        const engine::Vector3 g_doorBoxSize(2.0f, 2.0f, 2.0f);
        const engine::Vector3 g_zero(0.0f, 0.0f, 0.0f);

        struct StageRow
        {
            int stageIndex = 0;
            std::string mapEnvPrefab;
            float lightIntensity = 1.0f;
            int minMonsterCount = 3;
            int maxMonsterCount = 6;
            int anchorMonsterID = 0;
        };

        bool ParseStageRow(const std::vector<std::string>& fields, StageRow& out)
        {
            if (fields.size() < 6)
                return false;
            try
            {
                out.stageIndex = std::stoi(fields[0]);
                out.mapEnvPrefab = fields[1];
                out.lightIntensity = std::stof(fields[2]);
                out.minMonsterCount = std::stoi(fields[3]);
                out.maxMonsterCount = std::stoi(fields[4]);
                out.anchorMonsterID = std::stoi(fields[5]);
                if (out.minMonsterCount > out.maxMonsterCount)
                    out.minMonsterCount = out.maxMonsterCount;
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        bool GetStageDataFromCsv(int stageIndex, StageRow& out)
        {
            std::vector<StageRow> rows;
            auto parser = [](const std::vector<std::string>& fields, StageRow& row) -> bool
            {
                return ParseStageRow(fields, row);
            };
            if (!engine::CSVReader::Load<StageRow>(g_stageCsvPath, rows, parser))
                return false;
            for (const auto& r : rows)
            {
                if (r.stageIndex == stageIndex)
                {
                    out = r;
                    return true;
                }
            }
            return false;
        }

        void ApplyStageLightIntensity(float intensity)
        {
            engine::GameObject* mainLight = engine::GameObject::Find("MainLight");
            if (mainLight)
            {
                if (engine::Light* light = mainLight->GetComponent<engine::Light>())
                    light->SetIntensity(intensity);
            }
        }

        MonsterSpawner* FindMonsterSpawnerInHierarchy(engine::GameObject* root)
        {
            if (!root)
                return nullptr;
            if (auto* s = root->GetComponent<MonsterSpawner>())
                return s;
            engine::Transform* tr = root->GetTransform();
            if (!tr)
                return nullptr;
            for (engine::Transform* child : tr->GetChildren())
            {
                engine::GameObject* go = child->GetGameObject();
                if (MonsterSpawner* s = FindMonsterSpawnerInHierarchy(go))
                    return s;
            }
            return nullptr;
        }

        void SetActiveDoor(const char* name, bool isNextStage)
        {
            auto go = engine::GameObject::Find(name);
            if (!go) return;

            if (game::DoorTriggerScript* door = go->GetComponent<game::DoorTriggerScript>())
            {
                door->SetEventCallBack(isNextStage
                    ? []() { game::StageManager::Get().RequestNextStage(); }
                : []() { game::StageManager::Get().RequestGoToLobby(); });

                door->SetActivateDoor(true);
            }
        }
    }

    void StageManager::ClearStageState()
    {
        m_spawnedMonsters.clear();
        m_cleared = false;
    }

    void StageManager::AddRunCurrency(int ruby, int sapphire, int emerald)
    {
        if (ruby > 0) m_runRuby += ruby;
        if (sapphire > 0) m_runSapphire += sapphire;
        if (emerald > 0) m_runEmerald += emerald;
    }

    bool StageManager::GetMapEnvPrefabNameForStage(int stageIndex, std::string& outName) const
    {
        StageRow row;
        if (!GetStageDataFromCsv(stageIndex, row))
        {
            outName = g_defaultMapEnvPrefab ? g_defaultMapEnvPrefab : "";
            return outName[0] != '\0';
        }
        outName = row.mapEnvPrefab;
        return !outName.empty();
    }

    void StageManager::ComputeDifficulty(int stageIndex, int& targetScore, int& minCount, int& maxCount) const
    {
        const int baseScore = 100;
        const int scorePerStage = 20;
        const int baseMin = 3;
        const int baseMax = 6;
        const int countPerStage = 1;

        targetScore = baseScore + stageIndex * scorePerStage;
        minCount = baseMin + (stageIndex / 2) * countPerStage;
        maxCount = baseMax + (stageIndex / 2) * countPerStage;
        if (minCount > maxCount)
            minCount = maxCount;
    }

    void StageManager::BeginStage()
    {
        ClearStageState();
        // 씬 변경 시 엔진이 씬을 비우므로 여기서 Destroy 하지 않음. 참조만 비움.
        m_currentMapEnvRoot = engine::Ptr<engine::GameObject>();
        m_currentSpawnerRoot = engine::Ptr<engine::GameObject>();

        StageRow stageRow;
        const bool hasStageData = GetStageDataFromCsv(m_currentStage, stageRow);

        std::string mapPrefabName = hasStageData ? stageRow.mapEnvPrefab : "";
        if (!mapPrefabName.empty())
        {
            engine::GameObject* mapRoot = engine::Prefab::Instantiate(mapPrefabName);
            if (mapRoot)
                m_currentMapEnvRoot = engine::Ptr<engine::GameObject>(mapRoot);
        }

        if (hasStageData)
            ApplyStageLightIntensity(stageRow.lightIntensity);

        size_t idx = engine::Random::Int<size_t>(0, g_spawnerRootNames.size() - 1);
        const char* spawnerPrefabName = g_spawnerRootNames[idx];

        engine::GameObject* spawnerRoot = engine::Prefab::Instantiate(spawnerPrefabName);
        if (!spawnerRoot)
            return;

        m_currentSpawnerRoot = engine::Ptr<engine::GameObject>(spawnerRoot);

        MonsterSpawner* spawner = FindMonsterSpawnerInHierarchy(spawnerRoot);
        if (!spawner)
            return;

        int targetScore = 100, minCount = 3, maxCount = 6;
        ComputeDifficulty(m_currentStage, targetScore, minCount, maxCount);
        if (hasStageData)
        {
            minCount = stageRow.minMonsterCount;
            maxCount = stageRow.maxMonsterCount;
            if (minCount > maxCount)
                minCount = maxCount;
        }

        const int anchorMonsterID = hasStageData ? stageRow.anchorMonsterID : 0;
        spawner->SetManagedByStageManager(true);
        spawner->SetStageParams(targetScore, minCount, maxCount, anchorMonsterID);
        // SpawnNow() will be called from OnSpawnerReady when spawner's Start() runs this frame.
    }

    void StageManager::OnSpawnerReady(MonsterSpawner* spawner)
    {
        if (!spawner)
            return;
        spawner->SpawnNow();
        const auto& list = spawner->GetSpawnedMonsters();
        m_spawnedMonsters.assign(list.begin(), list.end());
    }

    void StageManager::Update()
    {
        bool hasMonsters = std::any_of(m_spawnedMonsters.begin(), m_spawnedMonsters.end(),
            [](const engine::Ptr<engine::GameObject>& p) { return static_cast<bool>(p); });

        if (!hasMonsters && !m_cleared)
        {
            m_cleared = true;
            engine::Scene* scene = engine::SceneManager::Get().GetScene();
            if (scene)
            {
                if (m_doorNextPosition != g_zero)
                    SetActiveDoor("StageDoor_Next", true);
                if (m_doorExitPosition != g_zero)
                    SetActiveDoor("StageDoor_Exit", false);   
            }
            // TODO: 보상 콜백 호출
        }
    }

    bool StageManager::ShouldFragileGaugeRise() const
    {
        return std::any_of(m_spawnedMonsters.begin(), m_spawnedMonsters.end(),
            [](const engine::Ptr<engine::GameObject>& p) { return static_cast<bool>(p); });
    }

    void StageManager::RequestGoToLobby()
    {
        UpgradeProgressManager::AddCurrency(m_runRuby, m_runSapphire, m_runEmerald);
        m_runRuby = m_runSapphire = m_runEmerald = 0;
        m_currentStage = 1;
        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        engine::SceneManager::Get().ChangeScene("01_READY_Lobby");
    }

    void StageManager::RequestNextStage()
    {
        ++m_currentStage;

        if (m_currentStage % 10 == 0)
        {
            game::LoadingScreenDrawer::OnSceneTransitionBegin();
            engine::SceneManager::Get().ChangeScene("01_READY_Boss");
            return;
        }

        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        engine::SceneManager::Get().ChangeScene("01_READY_Stage");
    }
}
