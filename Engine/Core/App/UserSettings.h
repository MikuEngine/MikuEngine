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
        bool isFullscreen = false;
        bool useVsync = true;
    };

    struct AudioSettings
    {
        float master = 1.0f;   // 0~1
        float bgm = 1.0f;      // 0~1
        float sfx = 1.0f;      // 0~1
        bool mute = false;
    };

    struct ControlSettings
    {
        float mouseSensitivity = 1.0f;  // 기획 단위로 조정
        bool invertY = false;
    };

    // 유저 세팅 : 윈도우, 오디오, 컨트롤 세팅
    struct UserSettings
    {
        int version = 1;               // 마이그레이션 대비
        WindowSettings window{};
        AudioSettings audio{};
        ControlSettings controls{};
        // 필요하면 gameplay/ui 등 계속 추가
    };
}