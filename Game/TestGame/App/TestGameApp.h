#pragma once

#include <Engine/Core/App/WinApp.h>

namespace game
{
    class TestGameApp :
        public engine::WinApp
    {
    public:
        TestGameApp();
        TestGameApp(const std::filesystem::path& settingFilePath);

    public:
        void Initialize() override;
    };
}
