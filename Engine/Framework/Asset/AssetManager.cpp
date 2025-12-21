#include "pch.h"
#include "AssetManager.h"

#include "Framework/Asset/FBXData.h"
#include "Framework/Asset/StaticMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/Asset/SkeletalMeshData.h"
#include "Framework/Asset/SkeletonData.h"
#include "Framework/Asset/AnimationData.h"
#include "Common/Utility/GeometryGenerator.h"

namespace engine
{
    void AssetManager::Initialize()
    {
        {
            auto meshData = GeometryGenerator::CreateCube();
            auto staticMeshData = std::make_shared<StaticMeshData>();
            staticMeshData->Create(std::move(meshData.vertices), std::move(meshData.indices));
            m_defaultStaticMeshes[static_cast<size_t>(DefaultStaticMeshType::Cube)] = staticMeshData;
        }

        {
            auto meshData = GeometryGenerator::CreateQuad();
            auto staticMeshData = std::make_shared<StaticMeshData>();
            staticMeshData->Create(std::move(meshData.vertices), std::move(meshData.indices));
            m_defaultStaticMeshes[static_cast<size_t>(DefaultStaticMeshType::Quad)] = staticMeshData;
        }
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

    std::shared_ptr<StaticMeshData> AssetManager::GetDefaultStaticMeshData(DefaultStaticMeshType type)
    {
        return m_defaultStaticMeshes[static_cast<size_t>(type)];
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
}