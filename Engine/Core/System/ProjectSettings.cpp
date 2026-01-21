#include "EnginePCH.h"
#include "ProjectSettings.h"

#include <fstream>
#include <filesystem>
#include <iomanip>

#include "Common/Utility/JsonHelper.h"

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
        std::filesystem::path path{ g_settingPath };
        if (!std::filesystem::exists(path))
        {
            return;
        }

        std::ifstream i{ path };
        if (i.is_open())
        {
            json root;
            i >> root;

            JsonGet(root, "SceneList", sceneList);
        }
    }
}