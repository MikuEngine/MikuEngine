#include "GamePCH.h"
#include "CSVReaderTest.h"

#include <Common/Utility/CSVReader.h>

namespace game
{
    struct TestData
    {
        int id = 0;
        std::string name;
        int level = 0;
        std::string say;
    };

    // 템플릿 특수화 사용 시
    template<>
    bool FromFields<TestData>(const std::vector<std::string>& fields, TestData& out)
    {
        if (fields.size() != 4) // TestData의 기본 멤버 변수 개수랑 다르면 잘못된거
        {
            return false;
        }

        try
        {
            out.id = std::stoi(fields[0]);
            out.name = fields[1];
            out.level = std::stoi(fields[2]);
            out.say = fields[3];
        }
        catch (...)
        {
            // std::stoi 등 변환 실패
            return false;
        }

        return true;
    }

    void CSVReaderTest::Start()
    {
        // 템플릿, 람다 활용한 CSV 읽기
        {
            std::vector<TestData> testData; // 데이터 담을 컨테이너 준비

            // 파서 람다 만듦
            auto parser = [](const std::vector<std::string>& fields, TestData& out)
                {
                    if (fields.size() != 4) // TestData의 기본 멤버 변수 개수랑 다르면 잘못된거
                    {
                        return false;
                    }

                    try
                    {
                        out.id = std::stoi(fields[0]);
                        out.name = fields[1];
                        out.level = std::stoi(fields[2]);
                        out.say = fields[3];
                    }
                    catch (...)
                    {
                        // std::stoi 등 변환 실패
                        return false;
                    }

                    return true;
                };

            if (engine::CSVReader::Load<TestData>("Resource/Data/TestData/TestData.txt", testData, parser)) // 성공 시 true 반환
            {
                for (auto& data : testData)
                {
                    LOG_PRINT("{}, {}, {}, {}", data.id, data.name, data.level, data.say);
                }
            }
        }

        // 템플릿 특수화 활용한 CSV 읽기
        {
            std::vector<TestData> testData; // 데이터 담을 컨테이너 준비

            // 파서 람다 만듦
            auto parser = [](const std::vector<std::string>& fields, TestData& out)
                {
                    return FromFields<TestData>(fields, out); // 특수화된 템플릿 호출
                };

            if (engine::CSVReader::Load<TestData>("Resource/Data/TestData/TestData.txt", testData, parser)) // 성공 시 true 반환
            {
                for (auto& data : testData)
                {
                    LOG_PRINT("{}, {}, {}, {}", data.id, data.name, data.level, data.say);
                }
            }
        }

        // CSV 파일을 모든 줄 읽기
        {
            std::vector<TestData> testData; // 데이터 담을 컨테이너 준비
            std::vector<std::string> lines;

            if (engine::CSVReader::LoadLines("Resource/Data/TestData/TestData.txt", lines)) // 성공 시 true 반환
            {
                for (size_t i = 1; i < lines.size(); ++i) // 첫번째 줄은 헤더라서 1부터 시작함
                {
                    std::vector<std::string> fields = engine::CSVReader::ParseLine(lines[i]);

                    if (fields.size() != 4) // TestData의 기본 멤버 변수 개수랑 다르면 잘못된거
                    {
                        break;
                    }

                    TestData data;

                    data.id = std::stoi(fields[0]);
                    data.name = fields[1];
                    data.level = std::stoi(fields[2]);
                    data.say = fields[3];

                    testData.push_back(std::move(data));
                }

                for (auto& data : testData)
                {
                    LOG_PRINT("{}, {}, {}, {}", data.id, data.name, data.level, data.say);
                }
            }
        }
    }

    void CSVReaderTest::OnGui()
    {
    }

    void CSVReaderTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void CSVReaderTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}