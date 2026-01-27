#pragma once

#include <string>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterData - 몬스터 데이터 구조체
    // 
    // CSV 형식: MonsterID, Type, Tier, Difficulty
    // 용도: CSV에서 파싱한 몬스터 정보 저장
    // ═══════════════════════════════════════════════════════════════
    struct MonsterData
    {
        int MonsterID;
        std::string Type;
        int Tier;
        int Difficulty;
        
        // 기본 생성자
        MonsterData()
            : MonsterID(0)
            , Type("")
            , Tier(0)
            , Difficulty(0)
        {}
        
        // 파라미터 생성자
        MonsterData(int id, const std::string& type, int tier, int difficulty)
            : MonsterID(id)
            , Type(type)
            , Tier(tier)
            , Difficulty(difficulty)
        {}
        
        // 비교 연산자 (난이도 기준 정렬용)
        bool operator<(const MonsterData& other) const
        {
            return Difficulty < other.Difficulty;
        }
        
        bool operator==(const MonsterData& other) const
        {
            return MonsterID == other.MonsterID;
        }
    };
}
