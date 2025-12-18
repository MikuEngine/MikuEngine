#pragma once

#include <vector>
#include <string>

namespace engine
{
    struct WindowSettings
    {
        int resolutionWidth = 1280;
        int resolutionHeight = 720;
        std::vector<std::string> supportedResolutions{ "1280x720", "1920x1080", "2560x1440", "3840x2160" };
        bool isFullScreen = false;
        bool useVsync = true;
    };

}