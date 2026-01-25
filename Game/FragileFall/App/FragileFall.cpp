#include "GamePCH.h"
#include "FragileFall.h"

namespace game
{
	engine::WindowSettings g_default{
		1920,
		1080,
		{ "1280x720", "1920x1080", "2560x1440", "3840x2160" },
		false,
		false
	};

	FragileFall::FragileFall()
		: engine::WinApp()
	{
	}

	FragileFall::FragileFall(const std::filesystem::path& settingFilePath)
		: engine::WinApp(settingFilePath, g_default)
	{
	}

	void FragileFall::Initialize()
	{
		engine::WinApp::Initialize();
	}
}