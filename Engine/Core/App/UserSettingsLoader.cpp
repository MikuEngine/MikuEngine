#include "EnginePCH.h"
#include "UserSettingsLoader.h"

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        AudioSettings,
        master, bgm, sfx, mute)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        ControlSettings,
        mouseSensitivity, invertY)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        UserSettings,
        version, window, audio, controls)

    namespace
    {
        static std::filesystem::path GetExeDir()
        {
            wchar_t buffer[MAX_PATH]{};
            GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            return std::filesystem::path(buffer).parent_path();
        }
    }

    std::filesystem::path UserSettingsLoader::GetDefaultPath()
    {
        return GetExeDir();
    }

    void UserSettingsLoader::Load(const std::filesystem::path& filePath, UserSettings& outSettings)
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
                outSettings = j.get<UserSettings>();
            }
            catch (...)
            {
                LOG_INFO("파일 오류. 기본 값으로 다시 저장 후 불러옴.");
                Save(filePath, outSettings);
            }
        }
    }

    bool UserSettingsLoader::Save(const std::filesystem::path& filePath, const UserSettings& settings)
    {
        if (filePath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(filePath.parent_path(), ec);
        }

        std::ofstream file(filePath);
        if (!file.is_open())
            return false;

        json j = settings;
        file << j.dump(4);
        return true;
    }

    bool engine::UserSettingsLoader::TryLoad(const std::filesystem::path& filePath, UserSettings& outSettings)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        try
        {
            json j;
            file >> j;
            outSettings = j.get<UserSettings>();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
}