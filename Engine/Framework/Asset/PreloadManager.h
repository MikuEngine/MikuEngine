#pragma once

#include <future>
#include <atomic>

#include "common/Utility/Singleton.h"

namespace engine
{
	class PreloadManager :
		public Singleton<PreloadManager>
	{
	private:
		json m_preloadData;
		bool m_isInitialized = false;
		std::atomic<bool> m_globalLoaded = false;

		std::future<void> m_loadingFuture;
		std::future<void> m_globalLoadingFuture;
		std::atomic<bool> m_isLoading = false;

		std::atomic<float> m_progress = 0.0f;
		std::atomic<int> m_totalAssetsToLoad = 0;
		std::atomic<int> m_loadedAssetsCount = 0;

	private:
		PreloadManager() = default;
		~PreloadManager() = default;

	public:
		void Initialize();
		void LoadSceneResourceAsync(const std::string& sceneName);
		void LoadSceneResourceSync(const std::string& sceneName);
		bool IsLoading() const;
		float GetProgress() const;

		bool IsGlobalLoaded() const;
		void LoadGlobalAsync();
		void LoadGlobalSync();

	private:
		void LoadSceneResourceWorker(const std::string& sceneName);
		void LoadGlobalWorker();
		void LoadAsset(const json& assetData, bool isGlobal);

	private:
		friend class Singleton<PreloadManager>;
	};
}