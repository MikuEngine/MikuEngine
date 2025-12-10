#pragma once

#include <Engine/Core/App/WinApp.h>

namespace game
{
    class TestGameApp :
        public engine::WinApp
    {
    public:
        TestGameApp();
        TestGameApp(const std::string& settingFilePath);
    };
}
