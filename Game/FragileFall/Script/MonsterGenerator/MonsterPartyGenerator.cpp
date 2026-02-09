#include "GamePCH.h"
#include "MonsterPartyGenerator.h"
#include <Common/Debug/Debug.h>
#include <algorithm>
#include <numeric>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생성자/소멸자
    // ═══════════════════════════════════════════════════════════════
    
    MonsterPartyGenerator::MonsterPartyGenerator()
    {
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 설정 메서드
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterPartyGenerator::SetCountRange(int minCount, int maxCount)
    {
        if (minCount > maxCount)
        {
            std::swap(minCount, maxCount);
        }
        m_minCount = std::max(1, minCount);
        m_maxCount = std::max(m_minCount, maxCount);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 초기화
    // ═══════════════════════════════════════════════════════════════
    
    bool MonsterPartyGenerator::LoadMonsterDB(const std::string& csvPath)
    {
        m_monsterDB.clear();
        
        // CSV 파서 함수 정의
        auto parser = [](const std::vector<std::string>& fields, MonsterData& out) -> bool
        {
            if (fields.size() < 11)
                return false;
            try
            {
                out.monsterID = std::stoi(fields[0]);
                out.monsterName = fields[1];
                out.type = static_cast<AttackType>(std::stoi(fields[2]));
                out.tier = static_cast<MonsterTier>(std::stoi(fields[3]));
                out.difficulty = std::stoi(fields[4]);
                out.minRuby = std::stoi(fields[5]);
                out.maxRuby = std::stoi(fields[6]);
                out.minSapphire = std::stoi(fields[7]);
                out.maxSapphire = std::stoi(fields[8]);
                out.minEmerald = std::stoi(fields[9]);
                out.maxEmerald = std::stoi(fields[10]);
                return true;
            }
            catch (...)
            {
                return false;
            }
        };
        
        // CSV 로드
        if (!engine::CSVReader::Load<MonsterData>(csvPath, m_monsterDB, parser))
        {
            LOG_ERROR("[MonsterPartyGenerator] Failed to load CSV: {}", csvPath);
            return false;
        }
        
        if (m_monsterDB.empty())
        {
            LOG_ERROR("[MonsterPartyGenerator] CSV file is empty or has no valid data: {}", csvPath);
            return false;
        }
        
        // 난이도 오름차순 정렬
        std::sort(m_monsterDB.begin(), m_monsterDB.end());
        
        LOG_PRINT("[MonsterPartyGenerator] Loaded {} monsters from {}", m_monsterDB.size(), csvPath);
        return true;
    }
    
    bool MonsterPartyGenerator::ValidateInputs() const
    {
        if (m_monsterDB.empty())
        {
            LOG_ERROR("[MonsterPartyGenerator] Monster DB is not loaded");
            return false;
        }
        
        if (m_targetScore <= 0)
        {
            LOG_ERROR("[MonsterPartyGenerator] TargetScore must be positive: {}", m_targetScore);
            return false;
        }
        
        if (m_minCount < 1 || m_maxCount < 1 || m_minCount > m_maxCount)
        {
            LOG_ERROR("[MonsterPartyGenerator] Invalid count range: [{}, {}]", m_minCount, m_maxCount);
            return false;
        }
        
        return true;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 메인 생성 메서드
    // ═══════════════════════════════════════════════════════════════
    
    std::vector<int> MonsterPartyGenerator::GenerateParty()
    {
        m_monsterParty.clear();
        m_remainingBudget = 0;
        
        // 입력 검증
        if (!ValidateInputs())
        {
            return std::vector<int>();
        }
        
        // Phase 1: 규모 결정 및 앵커 선정
        int partySize = DeterminePartySize();
        if (!SelectAnchor(m_anchorMonsterID))
        {
            LOG_ERROR("[MonsterPartyGenerator] Failed to select anchor monster");
            return std::vector<int>();
        }
        
        // Phase 2: 나머지 몬스터 초기화
        InitializeRemainingMonsters(partySize);
        m_remainingBudget = CalculateRemainingBudget(partySize);
        
        // 예산 검증 및 조정
        if (!ValidateBudget(partySize))
        {
            if (!TryAdjustPartySize(partySize))
            {
                LOG_PRINT("[MonsterPartyGenerator] WARNING: Budget validation failed, proceeding with RemainingBudget=0");
                m_remainingBudget = 0;
            }
            else
            {
                // 파티 크기가 조정되었으므로 다시 초기화
                InitializeRemainingMonsters(partySize);
                m_remainingBudget = CalculateRemainingBudget(partySize);
            }
        }
        
        // Phase 3: 잔여 승급 루프
        UpgradeLoop();
        
        // Phase 4: 셔플
        ShuffleParty();
        
        return m_monsterParty;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Phase 1: 규모 결정 및 앵커 선정
    // ═══════════════════════════════════════════════════════════════
    
    int MonsterPartyGenerator::DeterminePartySize() const
    {
        return engine::Random::Int(m_minCount, m_maxCount);
    }
    
    bool MonsterPartyGenerator::SelectAnchor(int monsterID)
    {
        // 앵커 ID가 0이거나 유효하지 않으면 최소 난이도 몬스터 사용
        if (monsterID <= 0)
        {
            const MonsterData* minMonster = FindMonsterByDifficulty(GetMinDifficulty());
            if (minMonster)
            {
                m_anchorMonster = *minMonster;
                LOG_PRINT("[MonsterPartyGenerator] Anchor not specified, using min difficulty monster: ID={}", m_anchorMonster.monsterID);
                return true;
            }
            return false;
        }
        
        // ID로 앵커 찾기
        const MonsterData* anchor = FindMonsterByID(monsterID);
        if (anchor)
        {
            m_anchorMonster = *anchor;
            return true;
        }
        
        // 앵커를 찾지 못한 경우 최소 난이도 몬스터로 대체
        const MonsterData* minMonster = FindMonsterByDifficulty(GetMinDifficulty());
        if (minMonster)
        {
            m_anchorMonster = *minMonster;
            LOG_PRINT("[MonsterPartyGenerator] WARNING: Anchor monster ID {} not found, using min difficulty monster: ID={}", 
                       monsterID, m_anchorMonster.monsterID);
            return true;
        }
        
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Phase 2: 나머지 몬스터 초기화
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterPartyGenerator::InitializeRemainingMonsters(int partySize)
    {
        m_monsterParty.clear();
        m_monsterParty.push_back(m_anchorMonster.monsterID);
        
        int minDifficulty = GetMinDifficulty();
        std::vector<int> minDifficultyMonsters = GetMonstersWithDifficulty(minDifficulty);
        
        if (minDifficultyMonsters.empty())
        {
            LOG_ERROR("[MonsterPartyGenerator] No monsters with min difficulty found");
            return;
        }
        
        // 나머지 N-1마리 초기화
        for (int i = 1; i < partySize; ++i)
        {
            size_t randomIndex = engine::Random::Int<size_t>(0, minDifficultyMonsters.size() - 1);
            m_monsterParty.push_back(minDifficultyMonsters[randomIndex]);
        }
    }
    
    int MonsterPartyGenerator::CalculateRemainingBudget(int partySize) const
    {
        int minDifficulty = GetMinDifficulty();
        int currentTotal = m_anchorMonster.difficulty + (minDifficulty * (partySize - 1));
        return m_targetScore - currentTotal;
    }
    
    bool MonsterPartyGenerator::ValidateBudget(int partySize) const
    {
        return CalculateRemainingBudget(partySize) >= 0;
    }
    
    bool MonsterPartyGenerator::TryAdjustPartySize(int& partySize)
    {
        // N을 1씩 감소시키며 MinCount까지 재시도
        while (partySize > m_minCount)
        {
            partySize--;
            if (ValidateBudget(partySize))
            {
                LOG_PRINT("[MonsterPartyGenerator] Adjusted party size to {} due to budget constraints", partySize);
                return true;
            }
        }
        
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Phase 3: 잔여 승급 루프
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterPartyGenerator::UpgradeLoop()
    {
        // 앵커를 제외한 나머지 몬스터들의 인덱스 목록 (승급 후보군)
        std::vector<size_t> candidateIndices;
        for (size_t i = 1; i < m_monsterParty.size(); ++i)
        {
            candidateIndices.push_back(i);
        }
        
        // 승급 루프
        while (m_remainingBudget > 0 && !candidateIndices.empty())
        {
            // 후보군 중 랜덤 선택
            size_t randomIndex = engine::Random::Int<size_t>(0, candidateIndices.size() - 1);
            size_t partyIndex = candidateIndices[randomIndex];
            
            int& monsterID = m_monsterParty[partyIndex];
            
            // 승급 시도
            if (!TryUpgradeMonster(monsterID, m_remainingBudget))
            {
                // 승급 불가능한 경우 후보군에서 제외
                candidateIndices.erase(candidateIndices.begin() + randomIndex);
            }
        }
    }
    
    int MonsterPartyGenerator::FindNextDifficulty(int currentDifficulty) const
    {
        // DB가 난이도 오름차순 정렬되어 있으므로, 현재 Difficulty보다 큰 첫 번째 몬스터 찾기
        for (const auto& monster : m_monsterDB)
        {
            if (monster.difficulty > currentDifficulty)
            {
                return monster.difficulty;
            }
        }
        
        return -1;  // 다음 난이도 없음
    }
    
    int MonsterPartyGenerator::CalculateUpgradeCost(int currentDifficulty, int nextDifficulty) const
    {
        if (nextDifficulty <= currentDifficulty)
        {
            return 0;
        }
        return nextDifficulty - currentDifficulty;
    }
    
    bool MonsterPartyGenerator::TryUpgradeMonster(int& monsterID, int& budget)
    {
        // 현재 몬스터 정보 찾기
        const MonsterData* currentMonster = FindMonsterByID(monsterID);
        if (!currentMonster)
        {
            return false;
        }
        
        int currentDifficulty = currentMonster->difficulty;
        int nextDifficulty = FindNextDifficulty(currentDifficulty);
        
        if (nextDifficulty < 0)
        {
            // 최대 난이도 도달
            return false;
        }
        
        int upgradeCost = CalculateUpgradeCost(currentDifficulty, nextDifficulty);
        
        if (budget >= upgradeCost)
        {
            // 승급 가능: 다음 난이도 몬스터 찾기
            const MonsterData* nextMonster = FindMonsterByDifficulty(nextDifficulty);
            if (nextMonster)
            {
                // 같은 난이도에 여러 몬스터가 있을 수 있으므로 랜덤 선택
                std::vector<int> candidates = GetMonstersWithDifficulty(nextDifficulty);
                if (!candidates.empty())
                {
                    size_t randomIndex = engine::Random::Int<size_t>(0, candidates.size() - 1);
                    monsterID = candidates[randomIndex];
                    budget -= upgradeCost;
                    return true;
                }
            }
        }
        
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Phase 4: 마무리
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterPartyGenerator::ShuffleParty()
    {
        // 앵커가 항상 첫 번째에 위치하지 않도록 셔플 (Fisher-Yates 알고리즘)
        for (size_t i = m_monsterParty.size(); i > 1; --i)
        {
            size_t j = engine::Random::Int<size_t>(0, i - 1);
            std::swap(m_monsterParty[i - 1], m_monsterParty[j]);
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 유틸리티 메서드
    // ═══════════════════════════════════════════════════════════════
    
    const MonsterData* MonsterPartyGenerator::FindMonsterByID(int id) const
    {
        for (const auto& monster : m_monsterDB)
        {
            if (monster.monsterID == id)
            {
                return &monster;
            }
        }
        return nullptr;
    }
    
    const MonsterData* MonsterPartyGenerator::FindMonsterByDifficulty(int difficulty) const
    {
        // DB가 정렬되어 있으므로 이진 탐색 가능하지만, 간단하게 선형 탐색
        for (const auto& monster : m_monsterDB)
        {
            if (monster.difficulty == difficulty)
            {
                return &monster;
            }
        }
        return nullptr;
    }
    
    int MonsterPartyGenerator::GetMinDifficulty() const
    {
        if (m_monsterDB.empty())
        {
            return 0;
        }
        return m_monsterDB[0].difficulty;  // 정렬되어 있으므로 첫 번째가 최소값
    }
    
    std::vector<int> MonsterPartyGenerator::GetMonstersWithDifficulty(int difficulty) const
    {
        std::vector<int> result;
        for (const auto& monster : m_monsterDB)
        {
            if (monster.difficulty == difficulty)
            {
                result.push_back(monster.monsterID);
            }
        }
        return result;
    }
    std::vector<int> MonsterPartyGenerator::GetMonstersByType(AttackType type) const
    {
        std::vector<int> result;
        for (const auto& monster : m_monsterDB)
        {
            if (monster.type == type)
            {
                result.push_back(monster.monsterID);
            }
        }


        return std::vector<int>();
    }
}
