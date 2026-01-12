#include "EnginePCH.h"
#include "AssetManager.h"

#include "Framework/Asset/FBXData.h"
#include "Framework/Asset/StaticMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/Asset/SkeletalMeshData.h"
#include "Framework/Asset/SkeletonData.h"
#include "Framework/Asset/AnimationData.h"
#include "Framework/Asset/SimpleMeshData.h"
#include "Framework/Asset/SpriteData.h"
#include "Framework/Asset/SpriteAnimationData.h"
#include "Framework/Asset/GeometryData.h"

namespace engine
{
    void AssetManager::Initialize()
    {
        m_globalCachedDatas.reserve(100);
        m_sceneCachedDatas.reserve(200);
        
        CreateGeometryData();
    }

    void AssetManager::CleanupSceneScope()
    {
        m_sceneCachedDatas.clear();

        m_tempAssets.fill(nullptr);
    }

    std::shared_ptr<StaticMeshData> AssetManager::GetOrCreateStaticMeshData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_staticMeshDatas.find(filePath); find != m_staticMeshDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto fbx = std::make_shared<FBXAssetData>();
        fbx->Create(FBXAssetKind::Static, filePath);

        m_staticMeshDatas[filePath] = fbx->GetStaticMeshData();
        m_materialDatas[filePath] = fbx->GetMaterialData();

        CacheData(fbx->GetStaticMeshData(), scope);
        CacheData(fbx->GetMaterialData(), scope);

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetStaticMeshData();
    }

    std::shared_ptr<MaterialData> AssetManager::GetOrCreateMaterialData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_materialDatas.find(filePath); find != m_materialDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto fbx = std::make_shared<FBXAssetData>();
        fbx->Create(FBXAssetKind::Static, filePath);

        m_staticMeshDatas[filePath] = fbx->GetStaticMeshData();
        m_materialDatas[filePath] = fbx->GetMaterialData();

        CacheData(fbx->GetStaticMeshData(), scope);
        CacheData(fbx->GetMaterialData(), scope);

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetMaterialData();
    }

    std::shared_ptr<SkeletalMeshData> AssetManager::GetOrCreateSkeletalMeshData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_skeletalMeshDatas.find(filePath); find != m_skeletalMeshDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto fbx = std::make_shared<FBXAssetData>();
        fbx->Create(FBXAssetKind::Skeletal, filePath);

        m_skeletalMeshDatas[filePath] = fbx->GetSkeletalMeshData();
        m_materialDatas[filePath] = fbx->GetMaterialData();
        m_animationDatas[filePath] = fbx->GetAnimationData();
        m_skeletonDatas[filePath] = fbx->GetSkeletonData();

        CacheData(fbx->GetSkeletalMeshData(), scope);
        CacheData(fbx->GetMaterialData(), scope);
        CacheData(fbx->GetAnimationData(), scope);
        CacheData(fbx->GetSkeletonData(), scope);

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetSkeletalMeshData();
    }

    std::shared_ptr<AnimationData> AssetManager::GetOrCreateAnimationData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_animationDatas.find(filePath); find != m_animationDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto fbx = std::make_shared<FBXAssetData>();
        fbx->Create(FBXAssetKind::Skeletal, filePath);

        m_skeletalMeshDatas[filePath] = fbx->GetSkeletalMeshData();
        m_materialDatas[filePath] = fbx->GetMaterialData();
        m_animationDatas[filePath] = fbx->GetAnimationData();
        m_skeletonDatas[filePath] = fbx->GetSkeletonData();

        CacheData(fbx->GetSkeletalMeshData(), scope);
        CacheData(fbx->GetMaterialData(), scope);
        CacheData(fbx->GetAnimationData(), scope);
        CacheData(fbx->GetSkeletonData(), scope);

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetAnimationData();
    }

    std::shared_ptr<SimpleMeshData> AssetManager::GetOrCreateSimpleMeshData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_simpleMeshDatas.find(filePath); find != m_simpleMeshDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto simpleMeshData = std::make_shared<SimpleMeshData>();
        simpleMeshData->Create(filePath);

        m_simpleMeshDatas[filePath] = simpleMeshData;

        CacheData(simpleMeshData, scope);

        return simpleMeshData;
    }

    std::shared_ptr<SkeletonData> AssetManager::GetOrCreateSkeletonData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_skeletonDatas.find(filePath); find != m_skeletonDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto fbx = std::make_shared<FBXAssetData>();
        fbx->Create(FBXAssetKind::Skeletal, filePath);

        m_skeletalMeshDatas[filePath] = fbx->GetSkeletalMeshData();
        m_materialDatas[filePath] = fbx->GetMaterialData();
        m_animationDatas[filePath] = fbx->GetAnimationData();
        m_skeletonDatas[filePath] = fbx->GetSkeletonData();

        CacheData(fbx->GetSkeletalMeshData(), scope);
        CacheData(fbx->GetMaterialData(), scope);
        CacheData(fbx->GetAnimationData(), scope);
        CacheData(fbx->GetSkeletonData(), scope);

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetSkeletonData();
    }

    std::shared_ptr<SpriteData> AssetManager::GetOrCreateSpriteData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_spriteDatas.find(filePath); find != m_spriteDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto spriteData = std::make_shared<SpriteData>();
        spriteData->Create(filePath);

        m_spriteDatas[filePath] = spriteData;

        CacheData(spriteData, scope);

        return spriteData;
    }

    std::shared_ptr<SpriteAnimationData> AssetManager::GetOrCreateSpriteAnimationData(const std::string& filePath, LifeScope scope)
    {
        if (auto find = m_spriteAnimationDatas.find(filePath); find != m_spriteAnimationDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto spriteAnimationData = std::make_shared<SpriteAnimationData>();
        spriteAnimationData->Create(filePath);

        m_spriteAnimationDatas[filePath] = spriteAnimationData;

        CacheData(spriteAnimationData, scope);

        return spriteAnimationData;
    }

    std::shared_ptr<GeometryData> AssetManager::GetGeometryData(const std::string& name)
    {
        if (auto find = m_geometryDatas.find(name); find != m_geometryDatas.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        assert(false && "없는 기본 도형. Initialize에 추가 필요");

        return nullptr;
    }

    void AssetManager::CreateGeometryData()
    {
        {
            auto geometryData = std::make_shared<GeometryData>();
            geometryData->Create(GeometryType::Quad);
            m_geometryDatas.emplace("DefaultQuad", geometryData);
            CacheData(geometryData, LifeScope::Global);
        }

        {
            auto geometryData = std::make_shared<GeometryData>();
            geometryData->Create(GeometryType::Cube);
            m_geometryDatas.emplace("DefaultCube", geometryData);
            CacheData(geometryData, LifeScope::Global);
        }

        {
            auto geometryData = std::make_shared<GeometryData>();
            geometryData->Create(GeometryType::Sphere);
            m_geometryDatas.emplace("DefaultSphere", geometryData);
            CacheData(geometryData, LifeScope::Global);
        }

        {
            auto geometryData = std::make_shared<GeometryData>();
            geometryData->Create(GeometryType::Plane);
            m_geometryDatas.emplace("DefaultPlane", geometryData);
            CacheData(geometryData, LifeScope::Global);
        }

        {
            auto geometryData = std::make_shared<GeometryData>();
            geometryData->Create(GeometryType::Cone);
            m_geometryDatas.emplace("DefaultCone", geometryData);
            CacheData(geometryData, LifeScope::Global);
        }
    }

    void AssetManager::CacheData(const std::shared_ptr<AssetData>& data, LifeScope scope)
    {
        switch (scope)
        {
        case LifeScope::Global:
            m_globalCachedDatas.push_back(data);
            break;

        case LifeScope::Scene:
            m_sceneCachedDatas.push_back(data);
            break;

        default:
            break;
        }
    }
}