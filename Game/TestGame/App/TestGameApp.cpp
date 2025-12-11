#include "pch.h"
#include "TestGameApp.h"

namespace game
{
	engine::WindowSettings g_default{ 1920, 1080, false, false, false, };

	TestGameApp::TestGameApp()
		: WinApp()
	{
	}

	TestGameApp::TestGameApp(const std::string& settingFilePath)
		: WinApp(settingFilePath, g_default)
	{
	}
}