#include "GamePCH.h"
#include "TestGameApp.h"

namespace game
{
	engine::WindowSettings g_default{
		1920,
		1080,
		{ "1280x720", "1920x1080", "2560x1440", "3840x2160" },
		false,
		false
	};

	TestGameApp::TestGameApp()
		: engine::WinApp()
	{
	}

	TestGameApp::TestGameApp(const std::filesystem::path& settingFilePath)
		: engine::WinApp(settingFilePath, g_default)
	{
	}

	void TestGameApp::Initialize()
	{
		engine::WinApp::Initialize();
	}
}