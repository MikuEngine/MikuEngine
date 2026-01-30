#pragma once

#include <string>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterData - 몬스터 데이터 구조체
    // 
    // CSV 형식: MonsterID, Type, Tier, Difficulty
    // 용도: CSV에서 파싱한 몬스터 정보 저장
    // 
	// Type: Dull(둔탁), Pointed(뾰족), Round(동글) 등
	// Tier: 0(Gray), 1(Green), 2(Blue), 3(Red), 4(Purple)
    // 
    // ═══════════════════════════════════════════════════════════════
    
    enum class AttackType
    { 
        Dull,
        Pointed,
        Round,
        Max
    };
    
    enum class MonsterTier
    { 
        Gray,
        Green,
        Blue,
        Red,
        Purple,
        Max
    };

    inline const char* GetAttackTypeStr(AttackType type)
    {
        switch (type)
        {
        case AttackType::Dull:    return "Dull";
        case AttackType::Pointed: return "Pointed";
        case AttackType::Round:   return "Round";
        default:                  return "None";
        }
    }

    inline const char* GetMonsterTierStr(MonsterTier tier)
    {
        switch (tier)
        {
        case MonsterTier::Gray:   return "Gray";
        case MonsterTier::Green:  return "Green";
        case MonsterTier::Blue:   return "Blue";
        case MonsterTier::Red:    return "Red";
        case MonsterTier::Purple: return "Purple";
        default:                  return "None";
        }
    }

    struct MonsterData
    {
        int MonsterID;
        std::string MonsterName;
        AttackType Type;
        MonsterTier Tier;
        int Difficulty;
        
        // 기본 생성자
        MonsterData()
            : MonsterID(0)
            , Type(AttackType::Dull)
            , Tier(MonsterTier::Gray)
            , Difficulty(0)
        {}
        
        // 파라미터 생성자
        MonsterData(int id, AttackType type, MonsterTier tier, int difficulty)
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
