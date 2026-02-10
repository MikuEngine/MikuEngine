#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <lz4.h> // LZ4 필수

namespace fs = std::filesystem;

#pragma pack(push, 1)
// VirtualFileSystem.h와 반드시 일치해야 함
struct PackHeader
{
    char signature[4];  // "MIKU"
    uint32_t version;   // 2
    uint32_t fileCount;
    uint64_t dataOffset;
};

struct FileEntry
{
    uint64_t pathHash;
    uint64_t offset;
    uint64_t compressedSize;    // 압축된 크기
    uint64_t uncompressedSize;  // 원본 크기

#ifdef _DEBUG
    char path[256];
#endif //_DEBUG
};

#pragma pack(pop)

uint64_t HashPath(const std::string& path)
{
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (char c : path) { hash ^= static_cast<uint64_t>(c); hash *= FNV_PRIME; }
    return hash;
}

std::string NormalizePath(const std::string& path, const std::string& baseDir)
{
    std::string normalized = path;
    if (normalized.find(baseDir) == 0) normalized = normalized.substr(baseDir.length());
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    while (normalized.size() >= 2 && normalized[0] == '.' && normalized[1] == '/') normalized = normalized.substr(2);
    if (!normalized.empty() && normalized[0] == '/') normalized = normalized.substr(1);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return std::tolower(c); });
    return normalized;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::string inputDir = "Resource";
    std::string outputFile = "Resource.pak";

    if (argc >= 2) inputDir = argv[1];
    if (argc >= 3) outputFile = argv[2];

    std::cout << "Asset Packer (LZ4 Support) - MikuEngine\n";

    if (!fs::exists(inputDir)) {
        std::cerr << "오류: 입력 폴더 없음: " << inputDir << "\n";
        return 1;
    }

    fs::path basePath = fs::absolute(inputDir);
    std::string baseDir = basePath.string();
    std::replace(baseDir.begin(), baseDir.end(), '\\', '/');
    if (baseDir.back() != '/') baseDir += '/';

    std::vector<std::pair<std::string, fs::path>> files;
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            std::string nPath = NormalizePath(entry.path().string(), baseDir);
            files.push_back({ nPath, entry.path() });
        }
    }

    std::cout << "파일 " << files.size() << "개 발견.\n";
    if (files.empty()) return 1;

    std::string tempFile = outputFile + ".tmp";
    std::ofstream outFile(tempFile, std::ios::binary);

    PackHeader header = {};
    std::memcpy(header.signature, "MIKU", 4);
    header.version = 2; // 버전 2
    header.fileCount = static_cast<uint32_t>(files.size());
    header.dataOffset = sizeof(PackHeader) + sizeof(FileEntry) * files.size();

    outFile.seekp(header.dataOffset);

    std::vector<FileEntry> entries;
    uint64_t currentOffset = header.dataOffset;

    for (const auto& [normalizedPath, filePath] : files)
    {
        FileEntry entry = {};
        entry.pathHash = HashPath(normalizedPath);
        entry.offset = currentOffset;

        // 원본 파일 읽기
        std::ifstream inFile(filePath, std::ios::binary | std::ios::ate);
        size_t srcSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        std::vector<char> srcBuffer(srcSize);
        inFile.read(srcBuffer.data(), srcSize);

        entry.uncompressedSize = srcSize;

        // 디버그용 경로 저장
#ifdef _DEBUG
        size_t pathLen = std::min(normalizedPath.length(), size_t(255));
        std::memcpy(entry.path, normalizedPath.c_str(), pathLen);
#endif //_DEBUG

        // 압축 시도
        int maxDestSize = LZ4_compressBound(static_cast<int>(srcSize));
        std::vector<char> compressedBuffer(maxDestSize);

        int compressedSize = LZ4_compress_default(
            srcBuffer.data(),
            compressedBuffer.data(),
            static_cast<int>(srcSize),
            maxDestSize
        );

        // 압축이 성공했고, 원본보다 작으면 압축 데이터 사용
        // (TGA가 이미 RLE 압축되어 있거나 파일이 너무 작으면 압축 효율이 없을 수 있음)
        if (compressedSize > 0 && compressedSize < static_cast<int>(srcSize))
        {
            entry.compressedSize = compressedSize;
            outFile.write(compressedBuffer.data(), compressedSize);
            currentOffset += compressedSize;
            std::cout << "[압축] " << normalizedPath << " (" << (int)(compressedSize * 100.0 / srcSize) << "%)\n";
        }
        else
        {
            // 압축하지 않음 (원본 저장)
            entry.compressedSize = srcSize;
            outFile.write(srcBuffer.data(), srcSize);
            currentOffset += srcSize;
            std::cout << "[원본] " << normalizedPath << " (Skip)\n";
        }

        entries.push_back(entry);
    }

    // 헤더와 엔트리 쓰기
    outFile.seekp(0);
    outFile.write(reinterpret_cast<const char*>(&header), sizeof(PackHeader));
    outFile.write(reinterpret_cast<const char*>(entries.data()), sizeof(FileEntry) * entries.size());
    outFile.close();

    // 파일 교체
    if (fs::exists(outputFile)) fs::remove(outputFile);
    fs::rename(tempFile, outputFile);

    std::cout << "\n패키징 완료: " << outputFile << "\n";
    return 0;
}