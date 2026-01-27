// CSVReader 템플릿 구현

namespace engine
{
    template<typename T>
    bool CSVReader::Load(const std::string& filepath,
                        std::vector<T>& outData,
                        std::function<bool(const std::vector<std::string>&, T&)> parser)
    {
        outData.clear();
        
        // CSV 파일의 모든 줄 로드
        std::vector<std::string> lines;
        if (!LoadLines(filepath, lines))
        {
            return false;
        }
        
        // 첫 번째 줄은 헤더로 스킵
        if (lines.empty())
        {
            return false;
        }
        
        size_t successCount = 0;
        size_t lineNumber = 1;  // 헤더 다음 줄부터 시작
        
        for (size_t i = 1; i < lines.size(); ++i, ++lineNumber)
        {
            // 빈 줄이나 주석 줄 스킵
            if (IsEmptyLine(lines[i]) || IsCommentLine(lines[i]))
            {
                continue;
            }
            
            // 필드로 분리
            std::vector<std::string> fields = ParseLine(lines[i]);
            
            // 파서 함수로 변환
            T data;
            if (parser(fields, data))
            {
                outData.push_back(data);
                successCount++;
            }
            else
            {
                LOG_ERROR("[CSVReader] Failed to parse line {}: {}", lineNumber, lines[i]);
            }
        }
        
        // 최소 1개 이상 파싱 성공 시 true
        return successCount > 0;
    }
    
    template<typename T>
    bool CSVReader::FromFields(const std::vector<std::string>& fields, T& out)
    {
        // 기본 구현: 항상 실패
        // 각 타입별로 특수화 필요
        (void)fields;
        (void)out;
        return false;
    }
}
