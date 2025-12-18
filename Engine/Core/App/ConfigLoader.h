#pragma once

#include <filesystem>

#include "WindowSettings.h"

namespace engine
{
    class ConfigLoader
    {
    public:
        static void Load(const std::filesystem::path& filePath, WindowSettings& outSettings);
        static void Save(const std::filesystem::path& filePath, const WindowSettings& settings);
    };
}
