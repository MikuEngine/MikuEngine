#include "EnginePCH.h"
#include "PreloadManager.h"

#include <fstream>

#include "Framework/Asset/AssetManager.h"
#include "Core/Graphics/Resource/ResourceManager.h"

namespace engine
{
	void PreloadManager::Initialize()
	{
		if (m_isInitialized)
		{
			return;
		}

		std::string configPath{ "Resource/Setting/Preload.json" };
		std::ifstream file{ configPath };

		if (file.is_open())
		{
			try
			{
				m_preloadData = json::parse(file);
			}
			catch (json::parse_error& e)
			{
				LOG_ERROR("Preload.json 파일 파싱 실패: {} - PreloadManager", e.what());

				return;
			}
		}
		else
		{
			LOG_ERROR("Preload.json 파일 열기 실패 - PreloadManager");
		}

		JsonArrayForEach(m_preloadData, "Global",
			[&](const json& asset)
			{
				this->LoadAsset(asset.value("Type", ""), asset.value("Path", ""), true);
			}
		);

		m_isInitialized = true;
	}

	void PreloadManager::LoadSceneResourceAsync(const std::string& sceneName)
	{
		if (m_isLoading)
		{
			LOG_ERROR("이미 로딩중입니다 - PreloadManager");
			return;
		}

		m_isLoading = true;
		m_progress = 0.0f;
		m_loadedAssetsCount = 0;
		m_totalAssetsToLoad = 0;

		m_loadingFuture = std::async(std::launch::async, &PreloadManager::LoadSceneResourceWorker, this, sceneName);
	}

	bool PreloadManager::IsLoading() const
	{
		return m_isLoading;
	}

	float PreloadManager::GetProgress() const
	{
		return m_progress;
	}

	void PreloadManager::LoadSceneResourceWorker(const std::string& sceneName)
	{
		if (!m_isInitialized || !m_preloadData.contains("Scenes"))
		{
			m_progress = 1.0f;
			m_isLoading = false;
			return;
		}

		const auto& scenes = m_preloadData["Scenes"];
		if (!scenes.contains(sceneName))
		{
			m_progress = 1.0f;
			m_isLoading = false;
			return;
		}

		const auto& sceneAssets = scenes[sceneName];
		m_totalAssetsToLoad = static_cast<int>(sceneAssets.size());

		if (m_totalAssetsToLoad == 0)
		{
			m_progress = 1.0f;
			m_isLoading = false;
			return;
		}

		for (const auto& asset : sceneAssets)
		{
			std::string type = asset.value("Type", "");
			std::string path = asset.value("Path", "");

			if (!type.empty() && !path.empty())
			{
				LoadAsset(type, path, false);
			}

			++m_loadedAssetsCount;
			m_progress = m_loadedAssetsCount / static_cast<float>(m_totalAssetsToLoad);
		}

		m_progress = 1.0f;
		m_isLoading = false;
	}

	void PreloadManager::LoadAsset(const std::string& type, const std::string& path, bool isGlobal)
	{
		LifeScope scope = isGlobal ? LifeScope::Global : LifeScope::Scene;

		if (type == "Texture")
		{
			ResourceManager::Get().GetOrCreateTexture(path, scope);
		}
		else if (type == "StaticMesh")
		{
			AssetManager::Get().GetOrCreateStaticMeshData(path, scope);
		}
		else if (type == "SkeletalMesh")
		{
			AssetManager::Get().GetOrCreateSkeletalMeshData(path, scope);
		}
		else if (type == "Animation")
		{
			AssetManager::Get().GetOrCreateAnimationData(path, scope);
		}
		else if (type == "SpriteData")
		{
			AssetManager::Get().GetOrCreateSpriteData(path, scope);
		}
		else if (type == "SpriteAnimation")
		{
			AssetManager::Get().GetOrCreateSpriteAnimationData(path, scope);
		}
	}
}