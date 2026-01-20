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

		std::future<void> m_loadingFuture;
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
		bool IsLoading() const;
		float GetProgress() const;

	private:
		void LoadSceneResourceWorker(const std::string& sceneName);
		void LoadAsset(const std::string& type, const std::string& path, bool isGlobal);

	private:
		friend class Singleton<PreloadManager>;
	};
}