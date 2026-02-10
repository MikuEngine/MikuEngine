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
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Scene/GameScene.h"

namespace game
{
    namespace
    {
        constexpr const char* g_stageCsvPath = "Resource/Data/Stages.csv";
        constexpr const char* g_defaultMapEnvPrefab = "";  // 비어 있으면 맵 환경 프리팹 미로드

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
            int difficulty = 0;
        };

        bool ParseStageRow(const std::vector<std::string>& fields, StageRow& out)
        {
            if (fields.size() < 7)
                return false;
            try
            {
                out.stageIndex = std::stoi(fields[0]);
                out.mapEnvPrefab = fields[1];
                out.lightIntensity = std::stof(fields[2]);
                out.minMonsterCount = std::stoi(fields[3]);
                out.maxMonsterCount = std::stoi(fields[4]);
                out.anchorMonsterID = std::stoi(fields[5]);
                out.difficulty = std::stoi(fields[6]);
                if (out.minMonsterCount > out.maxMonsterCount)
                    out.minMonsterCount = out.maxMonsterCount;
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        std::vector<StageRow> g_stageCache;

        bool LoadStageCacheIfNeeded()
        {
            if (!g_stageCache.empty())
                return true;
            auto parser = [](const std::vector<std::string>& fields, StageRow& row) -> bool
            {
                return ParseStageRow(fields, row);
            };
            return engine::CSVReader::Load<StageRow>(g_stageCsvPath, g_stageCache, parser);
        }

        bool GetStageDataFromCsv(int stageIndex, StageRow& out)
        {
            if (!LoadStageCacheIfNeeded())
                return false;
            for (const auto& r : g_stageCache)
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
        m_currentMapEnvRoot = engine::Ptr<engine::GameObject>();
        m_stageClearRewardRuby = m_stageClearRewardSapphire = m_stageClearRewardEmerald = 0;
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

    void StageManager::BeginStage()
    {
        ClearStageState();
        m_currentMapEnvRoot = engine::Ptr<engine::GameObject>();

        StageRow stageRow;
        const bool hasStageData = GetStageDataFromCsv(m_currentStage, stageRow);

        std::string mapPrefabName = hasStageData ? stageRow.mapEnvPrefab : "";
        engine::GameObject* mapRoot = nullptr;
        if (!mapPrefabName.empty())
        {
            mapRoot = engine::Prefab::Instantiate(mapPrefabName);
            if (mapRoot)
                m_currentMapEnvRoot = engine::Ptr<engine::GameObject>(mapRoot);
        }

        if (hasStageData)
            ApplyStageLightIntensity(stageRow.lightIntensity);

        MonsterSpawner* spawner = mapRoot ? FindMonsterSpawnerInHierarchy(mapRoot) : nullptr;
        if (spawner)
        {
            const int targetScore = hasStageData ? stageRow.difficulty : 100;
            int minCount = hasStageData ? stageRow.minMonsterCount : 3;
            int maxCount = hasStageData ? stageRow.maxMonsterCount : 6;
            if (minCount > maxCount)
                minCount = maxCount;
            const int anchorMonsterID = hasStageData ? stageRow.anchorMonsterID : 0;
            spawner->SetManagedByStageManager(true);
            spawner->SetStageParams(targetScore, minCount, maxCount, anchorMonsterID);
        }
    }

    void StageManager::OnSpawnerReady(MonsterSpawner* spawner)
    {
        if (!spawner)
            return;
        spawner->SpawnNow();
        const auto& list = spawner->GetSpawnedMonsters();
        m_spawnedMonsters.assign(list.begin(), list.end());
        spawner->ComputeAndReportStageClearReward();
    }

    void StageManager::SetStageClearReward(int ruby, int sapphire, int emerald)
    {
        m_stageClearRewardRuby = ruby;
        m_stageClearRewardSapphire = sapphire;
        m_stageClearRewardEmerald = emerald;
    }

    void StageManager::RegisterTutorialMonster(engine::GameObject* monster)
    {
        if (monster)
        {
            m_spawnedMonsters.push_back(engine::Ptr<engine::GameObject>(monster));
        }
    }

    void StageManager::Update()
    {
        bool hasMonsters = std::any_of(m_spawnedMonsters.begin(), m_spawnedMonsters.end(),
            [](const engine::Ptr<engine::GameObject>& p) { return static_cast<bool>(p); });

        if (!hasMonsters && !m_cleared)
        {
            m_cleared = true;
            m_runRuby += m_stageClearRewardRuby;
            m_runSapphire += m_stageClearRewardSapphire;
            m_runEmerald += m_stageClearRewardEmerald;
            m_stageClearRewardRuby = m_stageClearRewardSapphire = m_stageClearRewardEmerald = 0;

            // 스테이지 클리어 시 플레이어의 현재 프레자일 게이지를 저장
            engine::Scene* scene = engine::SceneManager::Get().GetScene();
            if (scene)
            {
                auto* playerGO = scene->FindGameObject("Player");
                if (playerGO)
                {
                    auto* playerScript = playerGO->GetComponent<PlayerControllerScript>();
                    if (playerScript)
                    {
                        SaveFragileGauge(playerScript->GetFragileGaugeCurrent());
                    }
                }

                if (scene->GetName() != "10_PROTO_Tutorial")
                {
                    SetActiveDoor("StageDoor_Next", true);
                    SetActiveDoor("StageDoor_Exit", false);
                }
            }
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
        ResetFragileGauge();  // 로비로 복귀 시 프레자일 게이지 초기화
        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        GameScene::Change(SceneID::Lobby);
    }

    void StageManager::RequestNextStage()
    {
        ++m_currentStage;

        if (m_currentStage % 10 == 0)
        {
            game::LoadingScreenDrawer::OnSceneTransitionBegin();
            //GameScene::Change(SceneID::Boss);
            return;
        }

        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        GameScene::Change(SceneID::Play);
    }
}
