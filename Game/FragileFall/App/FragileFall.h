#pragma once

#include <Engine/Core/App/WinApp.h>

namespace game
{
    class FragileFall :
        public engine::WinApp
    {
    public:
        FragileFall();
        FragileFall(const std::filesystem::path& settingFilePath);

    public:
        void Initialize() override;
    };
}
