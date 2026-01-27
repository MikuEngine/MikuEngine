#include "EnginePCH.h"
#include "CSVReader.h"

#include "Core/System/VirtualFileSystem.h"
#include "Common/Debug/Debug.h"

#include <sstream>
#include <cctype>

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 기본 파싱 기능
    // ═══════════════════════════════════════════════════════════════
    
    bool CSVReader::LoadLines(const std::string& filepath, 
                              std::vector<std::string>& outLines)
    {
        outLines.clear();
        
        // VirtualFileSystem을 통해 파일 로드
        std::vector<uint8_t> fileBuffer;
        if (!VirtualFileSystem::Get().LoadFile(filepath, fileBuffer))
        {
            LOG_ERROR("[CSVReader] Failed to load file: {}", filepath);
            return false;
        }
        
        if (fileBuffer.empty())
        {
            LOG_ERROR("[CSVReader] File is empty: {}", filepath);
            return false;
        }
        
        // UTF-8 BOM 제거 (EF BB BF)
        size_t startPos = 0;
        if (fileBuffer.size() >= 3 && 
            fileBuffer[0] == 0xEF && 
            fileBuffer[1] == 0xBB && 
            fileBuffer[2] == 0xBF)
        {
            startPos = 3;
        }
        
        // 버퍼를 문자열로 변환
        std::string content(reinterpret_cast<const char*>(fileBuffer.data() + startPos), 
                           fileBuffer.size() - startPos);
        
        // 줄 단위로 분리
        std::istringstream iss(content);
        std::string line;
        
        while (std::getline(iss, line))
        {
            // Windows 스타일 줄바꿈 처리 (\r\n -> \n)
            // UTF-8 멀티바이트 문자 안전 처리: 마지막 문자가 \r인지 확인
            if (!line.empty() && line.size() >= 1)
            {
                // 마지막 바이트가 \r인지 확인 (UTF-8 멀티바이트 문자 중간 바이트와 겹치지 않음)
                if (static_cast<unsigned char>(line.back()) == '\r')
                {
                    line.pop_back();
                }
            }
            
            outLines.push_back(line);
        }
        
        return !outLines.empty();
    }
    
    std::vector<std::string> CSVReader::ParseLine(const std::string& line, 
                                                  char delimiter)
    {
        std::vector<std::string> fields;
        std::string currentField;
        ParseState state = ParseState::IN_FIELD;
        
        for (size_t i = 0; i < line.length(); ++i)
        {
            char c = line[i];
            
            switch (state)
            {
            case ParseState::IN_FIELD:
                if (c == '"')
                {
                    // 따옴표 시작
                    state = ParseState::IN_QUOTED_FIELD;
                }
                else if (c == delimiter)
                {
                    // 필드 구분자
                    fields.push_back(Trim(currentField));
                    currentField.clear();
                }
                else
                {
                    currentField += c;
                }
                break;
                
            case ParseState::IN_QUOTED_FIELD:
                if (c == '"')
                {
                    // 닫는 따옴표 확인
                    if (i + 1 < line.length() && line[i + 1] == '"')
                    {
                        // 연속된 따옴표는 따옴표 문자로 처리 (RFC 4180 표준)
                        currentField += '"';
                        i++;  // 다음 따옴표 건너뛰기
                    }
                    else if (i + 1 < line.length() && line[i + 1] == delimiter)
                    {
                        // 다음 문자가 구분자면 필드 종료
                        fields.push_back(Trim(currentField));
                        currentField.clear();
                        state = ParseState::IN_FIELD;
                        i++;  // 구분자 건너뛰기
                    }
                    else if (i + 1 >= line.length() || std::isspace(static_cast<unsigned char>(line[i + 1])))
                    {
                        // 줄 끝이거나 공백이면 필드 종료
                        fields.push_back(Trim(currentField));
                        currentField.clear();
                        state = ParseState::IN_FIELD;
                    }
                    else
                    {
                        currentField += c;
                    }
                }
                else
                {
                    currentField += c;
                }
                break;
            }
        }
        
        // 마지막 필드 추가
        // currentField에 내용이 있으면 추가
        if (!currentField.empty())
        {
            fields.push_back(Trim(currentField));
        }
        // 줄 끝에 구분자가 있는 경우 (빈 필드 추가)
        else if (!line.empty() && line.back() == delimiter && state == ParseState::IN_FIELD)
        {
            fields.push_back(std::string());
        }
        // 따옴표가 닫히지 않은 경우도 필드로 추가
        else if (state == ParseState::IN_QUOTED_FIELD)
        {
            fields.push_back(Trim(currentField));
        }
        
        return fields;
    }
    
    std::string CSVReader::Trim(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }
        
        // UTF-8 안전 처리: 바이트 단위로 처리하되, 멀티바이트 문자 중간 바이트는 건드리지 않음
        // ASCII 공백 문자만 제거 (UTF-8 멀티바이트 문자는 그대로 유지)
        size_t start = 0;
        size_t end = str.length() - 1;
        
        // 앞쪽 ASCII 공백 제거
        while (start < str.length())
        {
            unsigned char c = static_cast<unsigned char>(str[start]);
            // ASCII 공백 문자만 체크 (0x00-0x7F 범위)
            if (c <= 0x7F && std::isspace(c))
            {
                start++;
            }
            else
            {
                break;  // 멀티바이트 문자 시작 또는 일반 문자
            }
        }
        
        // 뒤쪽 ASCII 공백 제거
        while (end > start)
        {
            unsigned char c = static_cast<unsigned char>(str[end]);
            // ASCII 공백 문자만 체크 (0x00-0x7F 범위)
            if (c <= 0x7F && std::isspace(c))
            {
                end--;
            }
            else
            {
                break;  // 멀티바이트 문자 또는 일반 문자
            }
        }
        
        if (start > end)
        {
            return std::string();
        }
        
        return str.substr(start, end - start + 1);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 헬퍼 메서드
    // ═══════════════════════════════════════════════════════════════
    
    bool CSVReader::IsEmptyLine(const std::string& line)
    {
        if (line.empty())
        {
            return true;
        }
        
        // UTF-8 안전 처리: ASCII 공백 문자만 체크
        return std::all_of(line.begin(), line.end(), 
            [](char c) 
            { 
                unsigned char uc = static_cast<unsigned char>(c);
                // ASCII 범위(0x00-0x7F)의 공백 문자만 체크
                return uc <= 0x7F && std::isspace(uc);
            });
    }
    
    bool CSVReader::IsCommentLine(const std::string& line)
    {
        std::string trimmed = Trim(line);
        return !trimmed.empty() && trimmed[0] == '#';
    }
}
