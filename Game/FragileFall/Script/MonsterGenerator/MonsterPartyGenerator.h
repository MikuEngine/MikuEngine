#pragma once

#include "MonsterData.h"
#include <Common/Utility/CSVReader.h>
#include <Common/Math/MathUtility.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterPartyGenerator - 앵커 기반 몬스터 파티 생성기
    // 
    // 핵심 로직:
    //   1. 생성할 몬스터 수(N)를 랜덤으로 결정
    //   2. 외부에서 설정한 앵커 몬스터 1마리 확정
    //   3. 나머지 N-1마리는 최소 등급에서 시작하여 예산 소진까지 승급
    // 
    // 사용법:
    //   MonsterPartyGenerator generator;
    //   generator.SetTargetScore(100);
    //   generator.SetCountRange(3, 5);
    //   generator.SetAnchorMonsterID(3);
    //   if (generator.LoadMonsterDB("Data/Monsters.csv"))
    //   {
    //       std::vector<int> party = generator.GenerateParty();
    //   }
    // ═══════════════════════════════════════════════════════════════
    class MonsterPartyGenerator
    {
    public:
        // ─────────────────────────────────────────────
        // 생성자/소멸자
        // ─────────────────────────────────────────────
        MonsterPartyGenerator();
        ~MonsterPartyGenerator() = default;
        
        // ─────────────────────────────────────────────
        // 설정 메서드
        // ─────────────────────────────────────────────
        void SetTargetScore(int score) { m_targetScore = score; }
        void SetCountRange(int minCount, int maxCount);
        void SetAnchorMonsterID(int id) { m_anchorMonsterID = id; }
        void SetCSVPath(const std::string& path) { m_csvFilePath = path; }
        
        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        bool LoadMonsterDB(const std::string& csvPath);
        bool ValidateInputs() const;
        
        // ─────────────────────────────────────────────
        // 메인 생성 메서드
        // ─────────────────────────────────────────────
        std::vector<int> GenerateParty();
        
        // ─────────────────────────────────────────────
        // Phase 1: 규모 결정 및 앵커 선정
        // ─────────────────────────────────────────────
        int DeterminePartySize() const;
        bool SelectAnchor(int monsterID);
        
        // ─────────────────────────────────────────────
        // Phase 2: 나머지 몬스터 초기화
        // ─────────────────────────────────────────────
        void InitializeRemainingMonsters(int partySize);
        int CalculateRemainingBudget(int partySize) const;
        bool ValidateBudget(int partySize) const;
        bool TryAdjustPartySize(int& partySize);
        
        // ─────────────────────────────────────────────
        // Phase 3: 잔여 승급 루프
        // ─────────────────────────────────────────────
        void UpgradeLoop();
        int FindNextDifficulty(int currentDifficulty) const;
        int CalculateUpgradeCost(int currentDifficulty, int nextDifficulty) const;
        bool TryUpgradeMonster(int& monsterID, int& budget);
        
        // ─────────────────────────────────────────────
        // Phase 4: 마무리
        // ─────────────────────────────────────────────
        void ShuffleParty();
        
        // ─────────────────────────────────────────────
        // 유틸리티 메서드
        // ─────────────────────────────────────────────
        const MonsterData* FindMonsterByID(int id) const;
        const MonsterData* FindMonsterByDifficulty(int difficulty) const;
        int GetMinDifficulty() const;
        std::vector<int> GetMonstersWithDifficulty(int difficulty) const;
        std::vector<int> GetMonstersByType(AttackType type) const;

        
        // ─────────────────────────────────────────────
        // 상태 조회
        // ─────────────────────────────────────────────
        bool IsDBLoaded() const { return !m_monsterDB.empty(); }
        int GetRemainingBudget() const { return m_remainingBudget; }
        const std::vector<MonsterData>& GetMonsterDB() const { return m_monsterDB; }
        
    private:
        // ─────────────────────────────────────────────
        // 입력 파라미터
        // ─────────────────────────────────────────────
        int m_targetScore = 100;
        int m_minCount = 3;
        int m_maxCount = 5;
        int m_anchorMonsterID = 0;
        std::string m_csvFilePath = "";
        
        // ─────────────────────────────────────────────
        // 내부 데이터
        // ─────────────────────────────────────────────
        std::vector<MonsterData> m_monsterDB;  // 난이도 오름차순 정렬
        MonsterData m_anchorMonster;
        std::vector<int> m_monsterParty;  // MonsterID 리스트
        int m_remainingBudget = 0;
    };
}
