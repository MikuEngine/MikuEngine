#include "EnginePCH.h"
#include "PreloadManager.h"

#include <fstream>

#include "Framework/Asset/AssetManager.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/System/VirtualFileSystem.h"
#include "Framework/System/SoundSystem.h"

namespace engine
{
    void PreloadManager::Initialize()
    {
        if (m_isInitialized)
        {
            return;
        }

        std::string configPath{ "Resource/Setting/Preload.setting" };
        
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        
        if (vfs.LoadFile(configPath, fileData))
        {
            try
            {
                m_preloadData = json::parse(fileData.begin(), fileData.end());
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

        m_globalLoaded = false;
        m_isInitialized = true;
    }

    bool PreloadManager::IsGlobalLoaded() const
    {
        return m_globalLoaded;
    }

    void PreloadManager::LoadGlobalSync()
    {
        if (m_globalLoaded)
        {
            return;
        }

        if (!m_preloadData.contains("Global"))
        {
            m_globalLoaded = true;
            return;
        }

        const auto& globalAssets = m_preloadData["Global"];
        for (const auto& asset : globalAssets)
        {
            LoadAsset(asset, true);
        }

        m_globalLoaded = true;
    }

    void PreloadManager::LoadGlobalAsync()
    {
        if (m_globalLoaded || m_isLoading)
        {
            return;
        }

        if (!m_preloadData.contains("Global"))
        {
            m_globalLoaded = true;
            return;
        }

        const auto& globalAssets = m_preloadData["Global"];
        m_totalAssetsToLoad = static_cast<int>(globalAssets.size());
        if (m_totalAssetsToLoad == 0)
        {
            m_globalLoaded = true;
            return;
        }

        m_isLoading = true;
        m_progress = 0.0f;
        m_loadedAssetsCount = 0;
        m_globalLoadingFuture = std::async(std::launch::async, &PreloadManager::LoadGlobalWorker, this);
    }

    void PreloadManager::LoadGlobalWorker()
    {
        if (!m_preloadData.contains("Global"))
        {
            m_globalLoaded = true;
            m_isLoading = false;
            return;
        }

        const auto& globalAssets = m_preloadData["Global"];
        for (const auto& asset : globalAssets)
        {
            LoadAsset(asset, true);
            ++m_loadedAssetsCount;
            m_progress = m_loadedAssetsCount / static_cast<float>(m_totalAssetsToLoad);
        }

        m_progress = 1.0f;
        m_globalLoaded = true;
        m_isLoading = false;
    }

    void PreloadManager::LoadSceneResourceAsync(const std::string& sceneName, std::function<void()> onSceneResourcesLoaded)
    {
        if (m_isLoading)
        {
            LOG_ERROR("이미 로딩중입니다 - PreloadManager");
            return;
        }

        m_onSceneResourcesLoadedCallback = std::move(onSceneResourcesLoaded);
        m_isLoading = true;
        m_progress = 0.0f;
        m_loadedAssetsCount = 0;
        m_totalAssetsToLoad = 0;

        m_loadingFuture = std::async(std::launch::async, &PreloadManager::LoadSceneResourceWorker, this, sceneName);
    }

    void PreloadManager::LoadSceneResourceSync(const std::string& sceneName)
    {
        if (m_isLoading)
        {
            LOG_ERROR("이미 로딩중입니다 - PreloadManager");
            return;
        }

        LoadGlobalSync();

        m_isLoading = true;
        m_progress = 0.0f;
        m_loadedAssetsCount = 0;
        m_totalAssetsToLoad = 0;

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
            LoadAsset(asset, false);

            ++m_loadedAssetsCount;
            m_progress = m_loadedAssetsCount / static_cast<float>(m_totalAssetsToLoad);
        }

        m_progress = 1.0f;
        m_isLoading = false;
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
            if (m_onSceneResourcesLoadedCallback)
            {
                m_onSceneResourcesLoadedCallback();
                m_onSceneResourcesLoadedCallback = nullptr;
            }
            m_isLoading = false;
            return;
        }

        const auto& scenes = m_preloadData["Scenes"];
        if (!scenes.contains(sceneName))
        {
            m_progress = 1.0f;
            if (m_onSceneResourcesLoadedCallback)
            {
                m_onSceneResourcesLoadedCallback();
                m_onSceneResourcesLoadedCallback = nullptr;
            }
            m_isLoading = false;
            return;
        }

        const auto& sceneAssets = scenes[sceneName];
        m_totalAssetsToLoad = static_cast<int>(sceneAssets.size());

        if (m_totalAssetsToLoad == 0)
        {
            m_progress = 1.0f;
            if (m_onSceneResourcesLoadedCallback)
            {
                m_onSceneResourcesLoadedCallback();
                m_onSceneResourcesLoadedCallback = nullptr;
            }
            m_isLoading = false;
            return;
        }

        for (const auto& asset : sceneAssets)
        {
            LoadAsset(asset, false);

            ++m_loadedAssetsCount;
            m_progress = m_loadedAssetsCount / static_cast<float>(m_totalAssetsToLoad);
        }

        m_progress = 1.0f;
        if (m_onSceneResourcesLoadedCallback)
        {
            m_onSceneResourcesLoadedCallback();
            m_onSceneResourcesLoadedCallback = nullptr;
        }
        m_isLoading = false;
    }

    void PreloadManager::LoadAsset(const json& assetData, bool isGlobal)
    {
        std::string type = assetData.value("Type", "");
        if (type.empty()) return;

        LifeScope scope = isGlobal ? LifeScope::Global : LifeScope::Scene;

        if (type == "SoundQue")
        {
            std::string key = assetData.value("Name", "");

            if (assetData.contains("Paths") && assetData["Paths"].is_array())
            {
                std::vector<std::string> paths;
                for (const auto& p : assetData["Paths"])
                {
                    paths.push_back(p);
                }

                std::string option = assetData.value("Option", "SFX");

                if (!key.empty() && !paths.empty())
                {
                    SoundSystem::Get().CreateRandomSound(key, paths, option, scope);
                }
            }
            return; 
        }

        std::string path = assetData.value("Path", "");

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
        else if (type == "Sound")
        {
            std::string name = assetData.value("Name", "");
            std::string option = assetData.value("Option", "SFX");

            std::string key = name.empty() ? path : name;
            AssetManager::Get().GetOrCreateSoundData(key, path, option, scope);
        }
    }
}