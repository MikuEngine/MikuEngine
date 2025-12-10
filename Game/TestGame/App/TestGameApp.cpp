#include "pch.h"
#include "TestGameApp.h"

namespace game
{
	engine::WindowSettings g_default{ 1280, 720, false, false, true, };

	TestGameApp::TestGameApp()
		: WinApp()
	{
	}

	TestGameApp::TestGameApp(const std::string& settingFilePath)
		: WinApp(settingFilePath, g_default)
	{
	}
}