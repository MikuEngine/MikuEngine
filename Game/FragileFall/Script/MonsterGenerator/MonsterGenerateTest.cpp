#include "GamePCH.h"
#include "MonsterGenerateTest.h"

#include <Common/Utility/CSVReader.h>

#include "Script/MonsterGenerator/MonsterPartyGenerator.h"
#include "Script/MonsterGenerator/MonsterData.h"

namespace game
{
    void MonsterGenerateTest::Start()
    {
        MonsterPartyGenerator generator;
        generator.SetTargetScore(700);
        generator.SetCountRange(3, 12);
        generator.SetAnchorMonsterID(3);
        if (generator.LoadMonsterDB("Resource/Data/TestData/TestMonsterData.txt"))
        {
            std::vector<int> party = generator.GenerateParty();


            // 템플릿, 람다 활용한 CSV 읽기
            {
                std::vector<MonsterData> monsterData; // 데이터 담을 컨테이너 준비

                // 파서 람다 만듦
                auto parser = [](const std::vector<std::string>& fields, MonsterData& out)
                    {
                        if (fields.size() != 4) // TestData의 기본 멤버 변수 개수랑 다르면 잘못된거
                        {
                            return false;
                        }

                        try
                        {
                            out.MonsterID = std::stoi(fields[0]);
                            out.Type = fields[1];
                            out.Tier = std::stoi(fields[2]);
                            out.Difficulty = std::stoi(fields[3]);
                        }
                        catch (...)
                        {
                            // std::stoi 등 변환 실패
                            return false;
                        }

                        return true;
                    };

                if (engine::CSVReader::Load<MonsterData>("Resource/Data/TestData/TestMonsterData.txt", monsterData, parser)) // 성공 시 true 반환
                {
                    int difficulty = 0;

                    for (auto id : party)
                    {
                        for (auto data : monsterData)
                        {
                            if (data.MonsterID == id)
                            {
                                difficulty += data.Difficulty;

                                LOG_PRINT("생성될 몬스터 ID: {}, 몬스터 이름: {} ,공격타입: {}, 티어(색상): {}, 난이도: {}", data.MonsterID, data.MonsterName, data.Type, data.Tier, data.Difficulty);

                                break;
                            }
                        }
                    }

                    LOG_PRINT("생성된 몬스터들의 난이도 총합: {}", difficulty);
                    LOG_PRINT("생성된 몬스터들의 갯수: {}", party.size());
                }
            }

        }
    }

    void MonsterGenerateTest::OnGui()
    {
    }

    void MonsterGenerateTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void MonsterGenerateTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}