#include "pch.h"
#include "TestGameApp.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/Component/StaticMeshRenderer.h"
#include "Framework/Object/Component/SkeletalMeshRenderer.h"
#include "Framework/Object/Component/SkeletalAnimator.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/Camera.h"

#include "Script/TestScript.h"

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