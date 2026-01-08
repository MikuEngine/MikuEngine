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

namespace engine
{
    void AssetManager::Initialize()
    {

    }

    std::shared_ptr<StaticMeshData> AssetManager::GetOrCreateStaticMeshData(const std::string& filePath)
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

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetStaticMeshData();
    }

    std::shared_ptr<MaterialData> AssetManager::GetOrCreateMaterialData(const std::string& filePath)
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

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetMaterialData();
    }

    std::shared_ptr<SkeletalMeshData> AssetManager::GetOrCreateSkeletalMeshData(const std::string& filePath)
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

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetSkeletalMeshData();
    }

    std::shared_ptr<AnimationData> AssetManager::GetOrCreateAnimationData(const std::string& filePath)
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

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetAnimationData();
    }

    std::shared_ptr<SimpleMeshData> AssetManager::GetOrCreateSimpleMeshData(const std::string& filePath)
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

        return simpleMeshData;
    }

    std::shared_ptr<SkeletonData> AssetManager::GetOrCreateSkeletonData(const std::string& filePath)
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

        m_tempAssets[m_tempAssetIndex++ % MAX_TEMP_ASSET] = fbx;

        return fbx->GetSkeletonData();
    }

    std::shared_ptr<SpriteData> AssetManager::GetOrCreateSpriteData(const std::string& filePath)
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

        return spriteData;
    }

    std::shared_ptr<SpriteAnimationData> AssetManager::GetOrCreateSpriteAnimationData(const std::string& filePath)
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

        return spriteAnimationData;
    }
}