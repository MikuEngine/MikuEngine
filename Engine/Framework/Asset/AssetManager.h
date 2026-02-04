#pragma once

#include "Common/Utility/Singleton.h"
#include "Common/Utility/CommonTypes.h"

namespace engine
{
    class AssetData;
    class StaticMeshData;
    class MaterialData;
    class SkeletalMeshData;
    class AnimationData;
    class SkeletonData;
    class SimpleMeshData;
    class SpriteData;
    class SpriteAnimationData;
    class GeometryData;
    class SoundData;
    class PrefabData;
	class SocketData;

    class AssetManager :
        public Singleton<AssetManager>
    {
    private:
        // assimp로 fbx로드할 때는 mesh, material, animation등 여러가지를 전부
        // 불러와야해서 fbx 로드할 때 임시 공간에 저장해둠 개별 파일로 분리하면
        // 필요없음
        static constexpr size_t MAX_TEMP_ASSET = 2;
        std::array<std::shared_ptr<AssetData>, MAX_TEMP_ASSET> m_tempAssets;
        size_t m_tempAssetIndex = 0;

        std::unordered_map<std::string, std::weak_ptr<StaticMeshData>> m_staticMeshDatas;
        std::unordered_map<std::string, std::weak_ptr<MaterialData>> m_materialDatas;
        std::unordered_map<std::string, std::weak_ptr<SkeletonData>> m_skeletonDatas;
        std::unordered_map<std::string, std::weak_ptr<SkeletalMeshData>> m_skeletalMeshDatas;
        std::unordered_map<std::string, std::weak_ptr<AnimationData>> m_animationDatas;
        std::unordered_map<std::string, std::weak_ptr<SimpleMeshData>> m_simpleMeshDatas;
        std::unordered_map<std::string, std::weak_ptr<SpriteData>> m_spriteDatas;
        std::unordered_map<std::string, std::weak_ptr<SpriteAnimationData>> m_spriteAnimationDatas;
        std::unordered_map<std::string, std::weak_ptr<GeometryData>> m_geometryDatas;
        std::unordered_map<std::string, std::weak_ptr<SoundData>> m_soundDatas;
        std::unordered_map<std::string, std::weak_ptr<PrefabData>> m_prefabDatas;
        std::unordered_map<std::string, std::weak_ptr<SocketData>> m_socketDatas;

        std::vector<std::shared_ptr<AssetData>> m_globalCachedDatas;
        std::vector<std::shared_ptr<AssetData>> m_sceneCachedDatas;

    private:
        AssetManager() = default;
        ~AssetManager() = default;

    public:
        void Initialize();
        void Shutdown();

        void CleanupSceneScope();

    public:
        std::shared_ptr<StaticMeshData> GetOrCreateStaticMeshData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<MaterialData> GetOrCreateMaterialData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<SkeletonData> GetOrCreateSkeletonData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<AnimationData> GetOrCreateAnimationData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<SimpleMeshData> GetOrCreateSimpleMeshData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<SkeletalMeshData> GetOrCreateSkeletalMeshData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<SpriteData> GetOrCreateSpriteData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<SpriteAnimationData> GetOrCreateSpriteAnimationData(const std::string& filePath, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<PrefabData> GetOrCreatePrefabData(const std::string& name, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<GeometryData> GetGeometryData(const std::string& name);
        std::shared_ptr<SoundData> GetOrCreateSoundData(const std::string& key, const std::string& filePath, const std::string& option, LifeScope scope = LifeScope::Owning);
        std::shared_ptr<SocketData> GetOrCreateSocketData(const std::string& filePath, LifeScope scope = LifeScope::Owning);

    private:
        void CreateGeometryData();
        void CacheData(const std::shared_ptr<AssetData>& data, LifeScope scope);

    private:
        friend class Singleton<AssetManager>;
    };
}