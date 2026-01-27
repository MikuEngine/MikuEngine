#pragma once

#include <filesystem>

#include "UserSettings.h"

namespace engine
{
    class UserSettingsLoader
    {
    public:
        static std::filesystem::path GetDefaultPath(); // Bin/Debug or Bin/Release/settings.config
        static void Load(const std::filesystem::path& filePath, UserSettings& inOutSettings);
        static bool Save(const std::filesystem::path& filePath, const UserSettings& settings);

    private:
        static bool TryLoad(const std::filesystem::path& filePath, UserSettings& outSettings);
    };
}
