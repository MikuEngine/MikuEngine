#include "GamePCH.h"
#include "FragileFall.h"

#include "Manager/LoadingScreenDrawer.h"
#include "Manager/TimeScaler.h"
#include <Engine/Framework/Scene/SceneManager.h>
#include <Engine/Framework/System/LoadingScreenRenderer.h>

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

		engine::LoadingScreenRenderer::Get().SetRenderCallback([](float progress)
			{
				LoadingScreenDrawer::Draw(progress);
			});

		engine::SceneManager::Get().RegisterOnSceneLoaded([]()
			{
				static bool once = false;
				if (!once)
				{
					once = true;
					LoadingScreenDrawer::OnFirstLoadFinished();
				}
			});

		m_onShutdownCallbacks.push_back([]() { LoadingScreenDrawer::OnShutdown(); });

		TimeScaler::Initialize();

		m_onGameplayUpdateCallbacks.push_back([]() { TimeScaler::Update(); });
	}
}