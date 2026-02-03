#pragma once

#include <Framework/Object/Component/Script.h>
#include "MonsterPartyGenerator.h"
#include <string>
#include <vector>

namespace game
{
    class MonsterSpawner :
        public engine::Script<MonsterSpawner>
    {
        REGISTER_SCRIPT(MonsterSpawner, Script)

    private:
        MonsterPartyGenerator m_partyGenerator;
        std::string m_monsterCsvPath;

        // 씬/스테이지 파라미터 (Start에서 Generator 설정용)
        int m_targetScore = 100;
        int m_minCount = 3;
        int m_maxCount = 5;
        int m_anchorMonsterID = 0;

    public:
        void Start() override;

        // ─── 1단계: 데이터·조회 ───
        bool LoadMonsterDB();
        std::string GetPrefabNameFromID(int monsterID) const;

        // ─── 2단계: 스폰 로직 ───
        engine::Vector3 GetRandomSpawnPosition() const;
        engine::GameObject* SpawnOne(int monsterID, const engine::Vector3& position);

        // ─── 3단계: 진입점 ───
        void SpawnParty(const std::vector<int>& partyIDs);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}