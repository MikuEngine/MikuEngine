#include "EnginePCH.h"
#include "VirtualFileSystem.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <lz4.h> // LZ4 헤더 포함 필수

namespace engine
{
    namespace
    {
        constexpr const char* PACK_SIGNATURE = "MIKU";
        constexpr uint32_t PACK_VERSION = 2; // 버전 2로 변경
    }

    VirtualFileSystem::VirtualFileSystem()
        : m_fileHandle(INVALID_HANDLE_VALUE)
        , m_fileMapping(nullptr)
        , m_mappedView(nullptr)
        , m_fileSize(0)
        , m_isPackedMode(false)
        , m_isDevelopmentMode(true)
    {
    }

    VirtualFileSystem::~VirtualFileSystem()
    {
        Unmount();
    }

    std::string VirtualFileSystem::NormalizePath(const std::string& path) const
    {
        std::string normalized = path;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        normalized.erase(0, normalized.find_first_not_of(" \t"));
        normalized.erase(normalized.find_last_not_of(" \t") + 1);
        if (normalized.size() >= 2 && normalized[0] == '.' && normalized[1] == '/')
        {
            normalized = normalized.substr(2);
        }
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return normalized;
    }

    uint64_t VirtualFileSystem::HashPath(const std::string& path) const
    {
        constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        constexpr uint64_t FNV_PRIME = 1099511628211ULL;
        uint64_t hash = FNV_OFFSET_BASIS;
        for (char c : path) { hash ^= static_cast<uint64_t>(c); hash *= FNV_PRIME; }
        return hash;
    }

    bool VirtualFileSystem::OpenMemoryMappedFile(const std::string& path)
    {
        m_fileHandle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fileHandle == INVALID_HANDLE_VALUE) return false;

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(m_fileHandle, &fileSize)) { CloseHandle(m_fileHandle); return false; }
        m_fileSize = static_cast<size_t>(fileSize.QuadPart);

        m_fileMapping = CreateFileMappingA(m_fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!m_fileMapping) { CloseHandle(m_fileHandle); return false; }

        m_mappedView = MapViewOfFile(m_fileMapping, FILE_MAP_READ, 0, 0, 0);
        if (!m_mappedView) { CloseHandle(m_fileMapping); CloseHandle(m_fileHandle); return false; }

        return true;
    }

    void VirtualFileSystem::CloseMemoryMappedFile()
    {
        if (m_mappedView) { UnmapViewOfFile(m_mappedView); m_mappedView = nullptr; }
        if (m_fileMapping) { CloseHandle(m_fileMapping); m_fileMapping = nullptr; }
        if (m_fileHandle != INVALID_HANDLE_VALUE) { CloseHandle(m_fileHandle); m_fileHandle = INVALID_HANDLE_VALUE; }
        m_fileSize = 0;
    }

    bool VirtualFileSystem::Mount(const std::string& packPath)
    {
        if (m_isDevelopmentMode || !std::filesystem::exists(packPath))
        {
            m_isPackedMode = false;
            return true;
        }

        m_packPath = packPath;
        m_fileTable.clear();

        if (!OpenMemoryMappedFile(packPath))
        {
            LOG_ERROR("패키지 파일 열기 실패: {} - VirtualFileSystem", packPath);
            return false;
        }

        const uint8_t* data = static_cast<const uint8_t*>(m_mappedView);
        const PackHeader* header = reinterpret_cast<const PackHeader*>(data);

        if (std::memcmp(header->signature, PACK_SIGNATURE, 4) != 0)
        {
            LOG_ERROR("잘못된 패키지 시그니처 - VirtualFileSystem");
            CloseMemoryMappedFile();
            return false;
        }

        if (header->version != PACK_VERSION)
        {
            LOG_ERROR("지원하지 않는 패키지 버전: {} (Engine expects {}) - VirtualFileSystem", header->version, PACK_VERSION);
            CloseMemoryMappedFile();
            return false;
        }

        const FileEntry* entries = reinterpret_cast<const FileEntry*>(data + sizeof(PackHeader));
        for (uint32_t i = 0; i < header->fileCount; ++i)
        {
            m_fileTable[entries[i].pathHash] = entries[i];
        }

        m_isPackedMode = true;
        LOG_INFO("패키지 마운트 완료 (v{}): {} (파일 {}개)", header->version, packPath, header->fileCount);
        return true;
    }

    void VirtualFileSystem::Unmount()
    {
        if (m_isPackedMode)
        {
            CloseMemoryMappedFile();
            m_fileTable.clear();
            m_isPackedMode = false;
            m_packPath.clear();
        }
    }

    bool VirtualFileSystem::FileExists(const std::string& path) const
    {
        std::string normalized = NormalizePath(path);
        if (m_isDevelopmentMode || !m_isPackedMode) return std::filesystem::exists(normalized);
        return m_fileTable.find(HashPath(normalized)) != m_fileTable.end();
    }

    bool VirtualFileSystem::ReadFromPack(const std::string& path, std::vector<uint8_t>& outBuffer)
    {
        std::string normalized = NormalizePath(path);
        uint64_t hash = HashPath(normalized);

        auto it = m_fileTable.find(hash);
        if (it == m_fileTable.end())
        {
            LOG_ERROR("파일을 찾을 수 없음: {} - VFS", path);
            return false;
        }

        const FileEntry& entry = it->second;

        if (entry.offset + entry.compressedSize > m_fileSize)
        {
            LOG_ERROR("파일 범위 초과: {} - VFS", path);
            return false;
        }

        const uint8_t* mappedData = static_cast<const uint8_t*>(m_mappedView);
        const uint8_t* fileSourceData = mappedData + entry.offset;

        // 원본 크기만큼 버퍼 할당
        outBuffer.resize(entry.uncompressedSize);

        if (entry.IsCompressed())
        {
            // LZ4 압축 해제
            int decompressedResult = LZ4_decompress_safe(
                reinterpret_cast<const char*>(fileSourceData),
                reinterpret_cast<char*>(outBuffer.data()),
                static_cast<int>(entry.compressedSize),
                static_cast<int>(entry.uncompressedSize)
            );

            if (decompressedResult < 0)
            {
                LOG_ERROR("압축 해제 실패: {} - VFS", path);
                return false;
            }
        }
        else
        {
            // 비압축 데이터 단순 복사
            std::memcpy(outBuffer.data(), fileSourceData, entry.uncompressedSize);
        }

        return true;
    }

    bool VirtualFileSystem::ReadFromFileSystem(const std::string& path, std::vector<uint8_t>& outBuffer)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        outBuffer.resize(size);
        file.read(reinterpret_cast<char*>(outBuffer.data()), size);
        return true;
    }

    bool VirtualFileSystem::LoadFile(const std::string& path, std::vector<uint8_t>& outBuffer)
    {
        outBuffer.clear();
        if (m_isDevelopmentMode || !m_isPackedMode) return ReadFromFileSystem(path, outBuffer);
        return ReadFromPack(path, outBuffer);
    }

    VirtualFileSystem& VirtualFileSystem::Get()
    {
        static VirtualFileSystem instance;
        return instance;
    }
}