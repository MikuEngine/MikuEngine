#pragma once

#include <functional>
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

		/// 워커에서 리소스 로드 완료 후 호출. 메인에서 설정, 워커에서 호출 후 클리어.
		std::function<void()> m_onSceneResourcesLoadedCallback;

	private:
		PreloadManager() = default;
		~PreloadManager() = default;

	public:
		void Initialize();
		/// onSceneResourcesLoaded: 워커에서 리소스 로드 직후 같은 스레드에서 호출됨(Scene::Load 등). nullptr면 호출 안 함.
		void LoadSceneResourceAsync(const std::string& sceneName, std::function<void()> onSceneResourcesLoaded = nullptr);
		void LoadSceneResourceSync(const std::string& sceneName);
		bool IsLoading() const;
		float GetProgress() const;
		/// 씬 로드 구간(0.5~1.0) 진행률. zeroToOne은 0~1 (GameObject 로드 비율).
		void SetSceneLoadProgress(float zeroToOne);
		/// 동기 씬 로드 시, Scene::Load() 완료 후 호출하여 프로그레스를 1.0으로 만듦 (에셋 0~0.5, 씬 로드 0.5~1.0)
		void SetSceneLoadComplete();

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