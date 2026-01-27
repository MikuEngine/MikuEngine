#include "GamePCH.h"
#include "FragileFall.h"

namespace game
{
	engine::UserSettings g_defaultSettings{
		.version = 1,
		.window = {1920, 1080,
		{ "1280x720", "1920x1080", "2560x1440", "3840x2160" },
		false, false},
		.audio = {1.0f, 1.0f, 1.0f, false},
		.controls = {1.2f, false}
	};

	FragileFall::FragileFall()
		: engine::WinApp()
	{
	}

	FragileFall::FragileFall(const std::filesystem::path& settingFilePath)
		: engine::WinApp(settingFilePath, g_defaultSettings)
	{
		
	}

	void FragileFall::Initialize()
	{
		m_windowName = "Fragile Fall";

		engine::WinApp::Initialize();
	}
}