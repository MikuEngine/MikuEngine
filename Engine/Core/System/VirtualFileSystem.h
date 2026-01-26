#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace engine
{
    // 패키지 파일 포맷 구조체
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
        char path[256];     // 디버그용 경로 (선택적)
    };

    // 가상 파일 시스템 클래스
    // 패키지 파일에서 에셋을 로드하거나, 개발 모드에서는 실제 파일 시스템에서 로드
    class VirtualFileSystem
    {
    public:
        VirtualFileSystem();
        ~VirtualFileSystem();

        // 패키지 파일 마운트 (초기화)
        bool Mount(const std::string& packPath);

        // 마운트 해제
        void Unmount();

        // 파일 로드 (경로를 주면 데이터를 읽어옴)
        // 반환: 성공 여부, outBuffer에 데이터 저장
        bool LoadFile(const std::string& path, std::vector<uint8_t>& outBuffer);

        // 파일이 존재하는지 확인
        bool FileExists(const std::string& path) const;

        // 개발 모드 설정 (에디터에서는 true, 릴리즈에서는 false)
        void SetDevelopmentMode(bool enabled) { m_isDevelopmentMode = enabled; }

        // 개발 모드인지 확인
        bool IsDevelopmentMode() const { return m_isDevelopmentMode; }

        // 싱글톤 인스턴스 접근
        static VirtualFileSystem& Get();

    private:
        // 경로 정규화 (백슬래시를 슬래시로, 대소문자 통일 등)
        std::string NormalizePath(const std::string& path) const;

        // FNV-1a 64bit 해시 함수
        uint64_t HashPath(const std::string& path) const;

        // 메모리 맵 파일 관련
        bool OpenMemoryMappedFile(const std::string& path);
        void CloseMemoryMappedFile();

        // 패키지 모드에서 파일 읽기
        bool ReadFromPack(const std::string& path, std::vector<uint8_t>& outBuffer);

        // 개발 모드에서 파일 읽기
        bool ReadFromFileSystem(const std::string& path, std::vector<uint8_t>& outBuffer);

    private:
        // 패키지 파일 정보
        HANDLE m_fileHandle;
        HANDLE m_fileMapping;
        void* m_mappedView;
        size_t m_fileSize;

        // 파일 테이블 (해시 -> 파일 정보)
        std::unordered_map<uint64_t, FileEntry> m_fileTable;

        // 패키지 모드 여부
        bool m_isPackedMode;

        // 개발 모드 여부 (에디터에서는 true)
        bool m_isDevelopmentMode;

        // 패키지 파일 경로
        std::string m_packPath;
    };
}
