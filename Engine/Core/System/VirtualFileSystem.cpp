#include "EnginePCH.h"
#include "VirtualFileSystem.h"

#include <fstream>
#include <algorithm>
#include <cctype>

namespace engine
{
    namespace
    {
        constexpr const char* PACK_SIGNATURE = "MIKU";
        constexpr uint32_t PACK_VERSION = 1;
    }

    VirtualFileSystem::VirtualFileSystem()
        : m_fileHandle(INVALID_HANDLE_VALUE)
        , m_fileMapping(nullptr)
        , m_mappedView(nullptr)
        , m_fileSize(0)
        , m_isPackedMode(false)
        , m_isDevelopmentMode(true) // 기본값은 개발 모드
    {
    }

    VirtualFileSystem::~VirtualFileSystem()
    {
        Unmount();
    }

    std::string VirtualFileSystem::NormalizePath(const std::string& path) const
    {
        std::string normalized = path;

        // 백슬래시를 슬래시로 변환
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        // 앞뒤 공백 제거
        normalized.erase(0, normalized.find_first_not_of(" \t"));
        normalized.erase(normalized.find_last_not_of(" \t") + 1);

        // 앞의 ./ 제거
        if (normalized.size() >= 2 && normalized[0] == '.' && normalized[1] == '/')
        {
            normalized = normalized.substr(2);
        }

        // 대소문자 통일 (Windows는 대소문자 구분 안 함)
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
            [](unsigned char c) { return std::tolower(c); });

        return normalized;
    }

    uint64_t VirtualFileSystem::HashPath(const std::string& path) const
    {
        // FNV-1a 64bit 해시
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

    bool VirtualFileSystem::OpenMemoryMappedFile(const std::string& path)
    {
        // 파일 열기
        m_fileHandle = CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        // 파일 크기 얻기
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(m_fileHandle, &fileSize))
        {
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        m_fileSize = static_cast<size_t>(fileSize.QuadPart);

        // 메모리 맵 파일 생성
        m_fileMapping = CreateFileMappingA(
            m_fileHandle,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (m_fileMapping == nullptr)
        {
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        // 뷰 매핑
        m_mappedView = MapViewOfFile(
            m_fileMapping,
            FILE_MAP_READ,
            0,
            0,
            0
        );

        if (m_mappedView == nullptr)
        {
            CloseHandle(m_fileMapping);
            CloseHandle(m_fileHandle);
            m_fileMapping = nullptr;
            m_fileHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        return true;
    }

    void VirtualFileSystem::CloseMemoryMappedFile()
    {
        if (m_mappedView != nullptr)
        {
            UnmapViewOfFile(m_mappedView);
            m_mappedView = nullptr;
        }

        if (m_fileMapping != nullptr)
        {
            CloseHandle(m_fileMapping);
            m_fileMapping = nullptr;
        }

        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
        }

        m_fileSize = 0;
    }

    bool VirtualFileSystem::Mount(const std::string& packPath)
    {
        // 개발 모드이거나 패키지 파일이 없으면 마운트하지 않음
        if (m_isDevelopmentMode || !std::filesystem::exists(packPath))
        {
            m_isPackedMode = false;
            return true; // 개발 모드에서는 성공으로 간주
        }

        m_packPath = packPath;
        m_fileTable.clear();

        // 메모리 맵 파일 열기
        if (!OpenMemoryMappedFile(packPath))
        {
            LOG_ERROR("패키지 파일 열기 실패: {} - VirtualFileSystem", packPath);
            return false;
        }

        // 헤더 읽기
        const uint8_t* data = static_cast<const uint8_t*>(m_mappedView);
        const PackHeader* header = reinterpret_cast<const PackHeader*>(data);

        // 시그니처 확인
        if (std::memcmp(header->signature, PACK_SIGNATURE, 4) != 0)
        {
            LOG_ERROR("잘못된 패키지 파일 시그니처 - VirtualFileSystem");
            CloseMemoryMappedFile();
            return false;
        }

        // 버전 확인
        if (header->version != PACK_VERSION)
        {
            LOG_ERROR("지원하지 않는 패키지 파일 버전: {} - VirtualFileSystem", header->version);
            CloseMemoryMappedFile();
            return false;
        }

        // 파일 엔트리 읽기
        const FileEntry* entries = reinterpret_cast<const FileEntry*>(data + sizeof(PackHeader));
        for (uint32_t i = 0; i < header->fileCount; ++i)
        {
            m_fileTable[entries[i].pathHash] = entries[i];
        }

        m_isPackedMode = true;
        LOG_INFO("패키지 파일 마운트 완료: {} (파일 {}개) - VirtualFileSystem", packPath, header->fileCount);

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

        if (m_isDevelopmentMode || !m_isPackedMode)
        {
            return std::filesystem::exists(normalized);
        }

        uint64_t hash = HashPath(normalized);
        return m_fileTable.find(hash) != m_fileTable.end();
    }

    bool VirtualFileSystem::ReadFromPack(const std::string& path, std::vector<uint8_t>& outBuffer)
    {
        std::string normalized = NormalizePath(path);
        uint64_t hash = HashPath(normalized);

        auto it = m_fileTable.find(hash);
        if (it == m_fileTable.end())
        {
            LOG_ERROR("패키지에서 파일을 찾을 수 없음: {} - VirtualFileSystem", path);
            return false;
        }

        const FileEntry& entry = it->second;

        // 범위 체크
        if (entry.offset + entry.size > m_fileSize)
        {
            LOG_ERROR("패키지 파일 범위 초과: {} - VirtualFileSystem", path);
            return false;
        }

        // 메모리 맵된 뷰에서 직접 읽기
        const uint8_t* data = static_cast<const uint8_t*>(m_mappedView);
        const uint8_t* fileData = data + entry.offset;

        outBuffer.resize(entry.size);
        std::memcpy(outBuffer.data(), fileData, entry.size);

        return true;
    }

    bool VirtualFileSystem::ReadFromFileSystem(const std::string& path, std::vector<uint8_t>& outBuffer)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return false;
        }

        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        outBuffer.resize(size);
        file.read(reinterpret_cast<char*>(outBuffer.data()), size);

        return true;
    }

    bool VirtualFileSystem::LoadFile(const std::string& path, std::vector<uint8_t>& outBuffer)
    {
        outBuffer.clear();

        // 개발 모드이거나 패키지 모드가 아니면 파일 시스템에서 읽기
        if (m_isDevelopmentMode || !m_isPackedMode)
        {
            return ReadFromFileSystem(path, outBuffer);
        }

        // 패키지 모드에서 읽기
        return ReadFromPack(path, outBuffer);
    }

    VirtualFileSystem& VirtualFileSystem::Get()
    {
        static VirtualFileSystem instance;
        return instance;
    }
}
