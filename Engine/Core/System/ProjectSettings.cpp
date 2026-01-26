#include "EnginePCH.h"
#include "ProjectSettings.h"

#include <fstream>
#include <filesystem>
#include <iomanip>

#include "Common/Utility/JsonHelper.h"
#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    namespace
    {
        const char* g_settingPath = "Resource/Setting/Project.setting";
    }

    void ProjectSettings::Save()
    {
        json root;

        root["SceneList"] = sceneList;

        std::filesystem::path path{ g_settingPath };

        if (path.has_parent_path())
        {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream o{ path };
        if (o.is_open())
        {
            o << std::setw(4) << root << std::endl;
        }
    }

    void ProjectSettings::Load()
    {
        // VFS를 통해 파일 로드 시도
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        
        if (vfs.LoadFile(g_settingPath, fileData))
        {
            try
            {
                json root = json::parse(fileData.begin(), fileData.end());
                JsonGet(root, "SceneList", sceneList);
            }
            catch (const json::parse_error& e)
            {
                LOG_ERROR("ProjectSettings 파일 파싱 실패: {} - ProjectSettings", e.what());
            }
        }
        else
        {
            // VFS에서 실패하면 파일 시스템에서 시도 (개발 모드)
            std::filesystem::path path{ g_settingPath };
            if (std::filesystem::exists(path))
            {
                std::ifstream i{ path };
                if (i.is_open())
                {
                    try
                    {
                        json root;
                        i >> root;
                        JsonGet(root, "SceneList", sceneList);
                    }
                    catch (const json::parse_error& e)
                    {
                        LOG_ERROR("ProjectSettings 파일 파싱 실패: {} - ProjectSettings", e.what());
                    }
                }
            }
        }
    }
}