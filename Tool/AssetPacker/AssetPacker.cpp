#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cctype>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace fs = std::filesystem;

// 패키지 파일 포맷 구조체 (VirtualFileSystem과 동일)
struct PackHeader
{
    char signature[4];  // "MIKU"
    uint32_t version;   // 1
    uint32_t fileCount;
    uint64_t dataOffset; // 실제 데이터가 시작되는 위치
};

struct FileEntry
{
    uint64_t pathHash;  // 경로 해시값 (FNV-1a 64bit)
    uint64_t offset;    // 패키지 파일 내에서의 시작 위치
    uint64_t size;      // 파일 크기
    char path[256];     // 디버그용 경로
};

// FNV-1a 64bit 해시 함수
uint64_t HashPath(const std::string& path)
{
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET_BASIS;
    for (char c : path)
    {
        hash ^= static_cast<uint64_t>(c);
        hash *= FNV_PRIME;
    }

    return hash;
}

// 경로 정규화
std::string NormalizePath(const std::string& path, const std::string& baseDir)
{
    std::string normalized = path;

    // 절대 경로를 상대 경로로 변환
    if (normalized.find(baseDir) == 0)
    {
        normalized = normalized.substr(baseDir.length());
    }

    // 백슬래시를 슬래시로 변환
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    // 앞의 ./ 제거
    while (normalized.size() >= 2 && normalized[0] == '.' && normalized[1] == '/')
    {
        normalized = normalized.substr(2);
    }

    // 앞의 / 제거
    if (!normalized.empty() && normalized[0] == '/')
    {
        normalized = normalized.substr(1);
    }

    // 대소문자 통일 (Windows는 대소문자 구분 안 함)
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return normalized;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // Windows 콘솔을 UTF-8로 설정
    SetConsoleOutputCP(65001);  // UTF-8 코드 페이지
    SetConsoleCP(65001);        // 입력도 UTF-8
#endif

    std::string inputDir = "Resource";
    std::string outputFile = "Resource.pak";

    // 명령줄 인자 처리
    if (argc >= 2)
    {
        inputDir = argv[1];
    }
    if (argc >= 3)
    {
        outputFile = argv[2];
    }

    std::cout << "Asset Packer - MikuEngine\n";
    std::cout << "입력 폴더: " << inputDir << "\n";
    std::cout << "출력 파일: " << outputFile << "\n\n";

    // 입력 폴더 확인
    if (!fs::exists(inputDir) || !fs::is_directory(inputDir))
    {
        std::cerr << "오류: 입력 폴더가 존재하지 않습니다: " << inputDir << "\n";
        return 1;
    }

    // 입력 폴더를 절대 경로로 변환
    fs::path basePath = fs::absolute(inputDir);
    std::string baseDir = basePath.string();
    std::replace(baseDir.begin(), baseDir.end(), '\\', '/');
    if (baseDir.back() != '/')
    {
        baseDir += '/';
    }

    // 파일 목록 수집
    std::vector<std::pair<std::string, fs::path>> files;
    for (const auto& entry : fs::recursive_directory_iterator(inputDir))
    {
        if (entry.is_regular_file())
        {
            std::string filePath = entry.path().string();
            std::string normalized = NormalizePath(filePath, baseDir);
            files.push_back({ normalized, entry.path() });
        }
    }

    std::cout << "파일 " << files.size() << "개 발견\n\n";

    if (files.empty())
    {
        std::cerr << "오류: 패키지할 파일이 없습니다.\n";
        return 1;
    }

    // 임시 파일에 데이터 쓰기
    std::string tempFile = outputFile + ".tmp";
    std::ofstream outFile(tempFile, std::ios::binary);
    if (!outFile.is_open())
    {
        std::cerr << "오류: 임시 파일을 열 수 없습니다: " << tempFile << "\n";
        return 1;
    }

    // 헤더 공간 예약 (나중에 덮어쓸 예정)
    PackHeader header = {};
    std::memcpy(header.signature, "MIKU", 4);
    header.version = 1;
    header.fileCount = static_cast<uint32_t>(files.size());
    header.dataOffset = sizeof(PackHeader) + sizeof(FileEntry) * files.size();

    // 헤더와 엔트리 공간 예약
    outFile.seekp(header.dataOffset);

    // 파일 엔트리 생성 및 데이터 쓰기
    std::vector<FileEntry> entries;
    uint64_t currentOffset = header.dataOffset;

    for (const auto& [normalizedPath, filePath] : files)
    {
        FileEntry entry = {};
        entry.pathHash = HashPath(normalizedPath);
        entry.offset = currentOffset;
        entry.size = fs::file_size(filePath);

        // 경로 복사 (최대 255자)
        size_t pathLen = std::min(normalizedPath.length(), size_t(255));
        std::memcpy(entry.path, normalizedPath.c_str(), pathLen);
        entry.path[pathLen] = '\0';

        entries.push_back(entry);

        // 파일 데이터 쓰기
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open())
        {
            std::cerr << "경고: 파일을 읽을 수 없습니다: " << filePath << "\n";
            continue;
        }

        std::vector<char> buffer(entry.size);
        inFile.read(buffer.data(), entry.size);
        outFile.write(buffer.data(), entry.size);

        currentOffset += entry.size;

        std::cout << "패키징: " << normalizedPath << " (" << entry.size << " bytes)\n";
    }

    // 헤더와 엔트리 쓰기
    outFile.seekp(0);
    outFile.write(reinterpret_cast<const char*>(&header), sizeof(PackHeader));
    outFile.write(reinterpret_cast<const char*>(entries.data()), sizeof(FileEntry) * entries.size());

    outFile.close();

    // 임시 파일을 최종 파일로 이동
    if (fs::exists(outputFile))
    {
        fs::remove(outputFile);
    }
    fs::rename(tempFile, outputFile);

    std::cout << "\n완료! 패키지 파일 생성: " << outputFile << "\n";
    std::cout << "총 파일 수: " << files.size() << "\n";
    std::cout << "패키지 크기: " << fs::file_size(outputFile) << " bytes\n";

    return 0;
}
