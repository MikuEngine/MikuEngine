#include "pch.h"
#include "TestGameApp.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/Component/StaticMeshRenderer.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/Camera.h"

#include "Script/TestScript.h"
#include "Script/EditorCameraController.h"

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

		auto OnEnter = []()
			{
				//engine::Scene* scene = engine::SceneManager::Get().GetCurrentScene();
				//
				//auto camera = scene->GetMainCamera();
				//camera->GetTransform()->SetLocalPosition({ 0.0f, 0.0f, -10.0f });
				//camera->GetGameObject()->AddComponent<EditorCameraController>();

				//engine::Ptr<engine::GameObject> gameObject = scene->CreateGameObject("TestGameObject");
				//engine::Ptr<TestScript> testScript = gameObject->AddComponent<TestScript>();
				//gameObject->AddComponent<engine::StaticMeshRenderer>("Resource/Model/char.fbx", "Shader/Pixel/BlinnPhongPS.hlsl");
				//auto transform = gameObject->GetTransform();
				//transform->SetLocalPosition({ 0.0f, 10.0f, 0.0f });


				//auto floor = scene->CreateGameObject("TestGameObject");
				//floor->AddComponent<engine::StaticMeshRenderer>("Resource/Model/Floor.fbx", "Shader/Pixel/BlinnPhongPS.hlsl");

			};

		engine::SceneManager::Get().CreateScene("SampleScene1", OnEnter);

		engine::SceneManager::Get().ChangeScene("SampleScene1");
	}
}