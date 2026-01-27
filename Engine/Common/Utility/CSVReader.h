#pragma once

#include <string>
#include <vector>
#include <functional>
#include <fstream>

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // CSVReader - 범용 CSV 파일 파싱 유틸리티
    // 
    // RFC 4180 표준 준수:
    //   - 따옴표 처리: "value, with comma"
    //   - 따옴표 안의 따옴표: "" (연속된 두 개의 따옴표)
    //   - 구분자: 쉼표(,) 기본값
    // 
    // 사용법:
    //   std::vector<MyData> data;
    //   auto parser = [](const std::vector<std::string>& fields, MyData& out) -> bool {
    //       // 파싱 로직
    //       return true;
    //   };
    //   engine::CSVReader::Load("Data/file.csv", data, parser);
    // ═══════════════════════════════════════════════════════════════
    class CSVReader
    {
    public:
        // ─────────────────────────────────────────────
        // 기본 CSV 파싱 (범용)
        // ─────────────────────────────────────────────
        
        // CSV 파일을 읽어서 각 줄을 문자열 벡터로 반환
        static bool LoadLines(const std::string& filepath, 
                             std::vector<std::string>& outLines);
        
        // CSV 한 줄을 필드로 분리 (RFC 4180 준수)
        // 기본 구분자: 쉼표(,) - RFC 4180 표준 및 게임 업계 표준
        // 유럽 지역에서는 세미콜론(;)도 사용되므로 커스터마이징 가능
        static std::vector<std::string> ParseLine(const std::string& line, 
                                                  char delimiter = ',');
        
        // 문자열 앞뒤 공백 제거
        static std::string Trim(const std::string& str);
        
        // ─────────────────────────────────────────────
        // 템플릿 기반 파싱
        // ─────────────────────────────────────────────
        
        // 템플릿: 파서 함수를 받아서 타입 T 리스트로 변환
        template<typename T>
        static bool Load(const std::string& filepath,
                        std::vector<T>& outData,
                        std::function<bool(const std::vector<std::string>&, T&)> parser);
        
        // 템플릿: 특수화를 통한 자동 파싱 (선택)
        // 각 타입별로 FromFields() 특수화 제공
        template<typename T>
        static bool FromFields(const std::vector<std::string>& fields, T& out);
        
    private:
        // 헬퍼 메서드
        static bool IsEmptyLine(const std::string& line);
        static bool IsCommentLine(const std::string& line);  // '#'으로 시작하는 줄
        
        // 파싱 상태 열거형
        enum class ParseState
        {
            IN_FIELD,           // 일반 필드 내부
            IN_QUOTED_FIELD     // 따옴표로 감싼 필드 내부
        };
    };
}

// 템플릿 구현은 헤더에 포함
#include "CSVReader.inl"
