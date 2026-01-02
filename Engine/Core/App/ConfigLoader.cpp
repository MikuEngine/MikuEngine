#include "pch.h"
#include "ConfigLoader.h"

#include <fstream>

namespace engine
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        WindowSettings,
        resolutionWidth,
        resolutionHeight,
        supportedResolutions,
        isFullscreen,
        useVsync)

        void ConfigLoader::Load(const std::filesystem::path& filePath, WindowSettings& outSettings)
    {
        if (!std::filesystem::exists(filePath))
        {
            Save(filePath, outSettings);
            return;
        }

        std::ifstream file(filePath);
        if (file.is_open())
        {
            try
            {
                json j;
                file >> j;
                outSettings = j.get<WindowSettings>();
            }
            catch (...)
            {
                LOG_INFO("파일 오류. 기본 값으로 다시 저장 후 불러옴.");
                Save(filePath, outSettings);
            }
        }
    }

    void ConfigLoader::Save(const std::filesystem::path& filePath, const WindowSettings& settings)
    {
        if (filePath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(filePath.parent_path(), ec);
        }

        std::ofstream file(filePath);
        if (file.is_open())
        {
            json j = settings;
            file << j.dump(4);
        }
    }
}