#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>
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

        /// 포인트 중심 반경. 0이면 포인트에 정확히 스폰, >0이면 XZ 원 안에 랜덤 스폰.
        float m_spawnRadius = 0.0f;

        // 포인트 10개 고정, 구역 3개(각 3/4/3). Spawner 자식=구역, 구역 자식=포인트. 중복 배치 불가, 구역 위치에는 미스폰.
        std::vector<engine::Transform*> m_allPoints;   // [zone0 포인트 3, zone1 포인트 4, zone2 포인트 3]
        std::vector<bool> m_pointUsed;                 // 포인트별 사용 여부 (SpawnParty 시작 시 초기화)
        std::vector<size_t> m_zoneStartIndex;          // [0, 3, 7, 10] — 구역 k = [m_zoneStartIndex[k], m_zoneStartIndex[k+1])
        size_t m_nextZoneIndex = 0;                   // 다음에 시도할 구역 (순환)

        /// StageManager 생존 체크용. SpawnParty() 시작 시 clear, SpawnOne() 성공 시 push.
        std::vector<engine::Ptr<engine::GameObject>> m_spawnedMonsters;

        /// true면 Start()에서 스폰하지 않음. StageManager가 SetStageParams() 후 SpawnNow() 호출.
        bool m_managedByStageManager = false;

    public:
        void Start() override;

        // ─── 1단계: 데이터·조회 ───
        bool LoadMonsterDB();
        std::string GetPrefabNameFromID(int monsterID) const;

        // ─── 2단계: 스폰 로직 (구역 순환 + 구역 내 랜덤 포인트) ───
        engine::Vector3 GetNextSpawnPosition();
        engine::GameObject* SpawnOne(int monsterID, const engine::Vector3& position);

        // ─── 3단계: 진입점 ───
        void SpawnParty(const std::vector<int>& partyIDs);

        /// StageManager 생존 체크용. SpawnParty()/SpawnNow() 직후 호출해 Ptr 목록 복사.
        const std::vector<engine::Ptr<engine::GameObject>>& GetSpawnedMonsters() const { return m_spawnedMonsters; }

        void SetManagedByStageManager(bool managed) { m_managedByStageManager = managed; }

        void SetStageParams(int targetScore, int minCount, int maxCount, int anchorMonsterID);

        /// StageManager가 SetStageParams() 후 호출. Start()와 동일 스폰 로직(DB 로드, Generator 설정, GenerateParty, SpawnParty).
        void SpawnNow();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}