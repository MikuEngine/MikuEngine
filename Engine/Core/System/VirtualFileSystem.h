#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace engine
{
#pragma pack(push, 1)
    // 패키지 파일 포맷 구조체
    struct PackHeader
    {
        char signature[4];  // "MIKU"
        uint32_t version;   // 2 (압축 지원 버전으로 변경)
        uint32_t fileCount;
        uint64_t dataOffset; // 실제 데이터가 시작되는 위치
    };

    struct FileEntry
    {
        uint64_t pathHash;          // 경로 해시값
        uint64_t offset;            // 패키지 파일 내 데이터 시작 위치
        uint64_t compressedSize;    // 압축된 크기 (패키지에 저장된 크기)
        uint64_t uncompressedSize;  // 원본 크기 (메모리에 로드될 크기)

#ifdef _DEBUG
        char path[256];             // 디버그용 경로
#endif //_DEBUG

        // 압축 여부 확인 (저장된 크기가 원본보다 작으면 압축된 것)
        bool IsCompressed() const { return compressedSize != uncompressedSize; }
    };
#pragma pack(pop)

    class VirtualFileSystem
    {
    public:
        VirtualFileSystem();
        ~VirtualFileSystem();

        bool Mount(const std::string& packPath);
        void Unmount();
        bool LoadFile(const std::string& path, std::vector<uint8_t>& outBuffer);
        bool FileExists(const std::string& path) const;

        void SetDevelopmentMode(bool enabled) { m_isDevelopmentMode = enabled; }
        bool IsDevelopmentMode() const { return m_isDevelopmentMode; }

        static VirtualFileSystem& Get();

    private:
        std::string NormalizePath(const std::string& path) const;
        uint64_t HashPath(const std::string& path) const;
        bool OpenMemoryMappedFile(const std::string& path);
        void CloseMemoryMappedFile();

        bool ReadFromPack(const std::string& path, std::vector<uint8_t>& outBuffer);
        bool ReadFromFileSystem(const std::string& path, std::vector<uint8_t>& outBuffer);

    private:
        HANDLE m_fileHandle;
        HANDLE m_fileMapping;
        void* m_mappedView;
        size_t m_fileSize;

        std::unordered_map<uint64_t, FileEntry> m_fileTable;
        bool m_isPackedMode;
        bool m_isDevelopmentMode;
        std::string m_packPath;
    };
}