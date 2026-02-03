#pragma once

#include <Framework/Object/Component/Script.h>
#include "MonsterPartyGenerator.h"
#include <string>

namespace game
{
    class MonsterSpawner :
        public engine::Script<MonsterSpawner>
    {
        REGISTER_SCRIPT(MonsterSpawner, Script)

    private:
        MonsterPartyGenerator m_partyGenerator;
        std::string m_monsterCsvPath;

    public:
        //void Awake() override;
        //void Start() override;
        //void Update() override;

        // ─── 1단계: 데이터·조회 (ID → 프리팹 이름 = MonsterName 임시) ───
        bool LoadMonsterDB();
        std::string GetPrefabNameFromID(int monsterID) const;

        // ─── 2단계: 스폰 로직 ───
        engine::Vector3 GetRandomSpawnPosition() const;
        engine::GameObject* SpawnOne(int monsterID, const engine::Vector3& position);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}