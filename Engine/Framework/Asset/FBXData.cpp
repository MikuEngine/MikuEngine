#include "EnginePCH.h"
#include "FBXData.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Framework/Asset/StaticMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/Asset/SkeletalMeshData.h"
#include "Framework/Asset/SkeletonData.h"
#include "Framework/Asset/AnimationData.h"
#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    void FBXAssetData::Create(FBXAssetKind kind, const std::string& filePath)
    {
        switch (kind)
        {
        case FBXAssetKind::Static:
            LoadStaticMesh(filePath);
            break;

        case FBXAssetKind::Skeletal:
            LoadSkeletalMesh(filePath);
            break;

        case FBXAssetKind::Animation:
            LoadAnimation(filePath);
            break;
        }
    }

    std::shared_ptr<StaticMeshData> FBXAssetData::GetStaticMeshData() const
    {
        return m_staticMesh;
    }

    std::shared_ptr<MaterialData> FBXAssetData::GetMaterialData() const
    {
        return m_material;
    }

    std::shared_ptr<SkeletalMeshData> FBXAssetData::GetSkeletalMeshData() const
    {
        return m_skeletalMesh;
    }

    std::shared_ptr<SkeletonData> FBXAssetData::GetSkeletonData() const
    {
        return m_skeleton;
    }

    std::shared_ptr<AnimationData> FBXAssetData::GetAnimationData() const
    {
        return m_animation;
    }

    void FBXAssetData::LoadStaticMesh(const std::string& filePath)
    {
        Assimp::Importer importer;

        const unsigned int importFlags =
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_GenUVCoords |
            aiProcess_CalcTangentSpace |
            aiProcess_ConvertToLeftHanded/* |
            aiProcess_PreTransformVertices*/;

        // VFS를 통해 파일 로드
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        if (!vfs.LoadFile(filePath, fileData))
        {
            LOG_ERROR("FBX 파일 로드 실패: {} - FBXAssetData", filePath);
            return;
        }

        const aiScene* scene = importer.ReadFileFromMemory(
            fileData.data(),
            fileData.size(),
            importFlags,
            filePath.c_str());

        aiMatrix4x4 rotation;
        aiMatrix4x4::RotationY(DirectX::XM_PI, rotation);

        scene->mRootNode->mTransformation = rotation * scene->mRootNode->mTransformation;

        importer.ApplyPostProcessing(aiProcess_PreTransformVertices);

        m_staticMesh = std::make_shared<StaticMeshData>();
        m_material = std::make_shared<MaterialData>();

        m_staticMesh->Create(scene);
        m_material->Create(scene, filePath);
    }

    void FBXAssetData::LoadSkeletalMesh(const std::string& filePath)
    {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        const unsigned int importFlags =
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_GenUVCoords |
            aiProcess_CalcTangentSpace |
            aiProcess_LimitBoneWeights |
            aiProcess_ConvertToLeftHanded;

        // VFS를 통해 파일 로드
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        if (!vfs.LoadFile(filePath, fileData))
        {
            LOG_ERROR("FBX 파일 로드 실패: {} - FBXAssetData", filePath);
            return;
        }

        const aiScene* scene = importer.ReadFileFromMemory(
            fileData.data(),
            fileData.size(),
            importFlags,
            filePath.c_str());

        // bone 생성
        m_skeleton = std::make_shared<SkeletonData>();
        m_skeleton->Create(scene);

        bool isRigid = scene->mMeshes[0]->mNumBones == 0;

        // mesh 생성
        m_skeletalMesh = std::make_shared<SkeletalMeshData>();
        m_skeletalMesh->Create(scene, m_skeleton, isRigid);

        // material 생성
        m_material = std::make_shared<MaterialData>();
        m_material->Create(scene, filePath);
    }

    void FBXAssetData::LoadAnimation(const std::string& filePath)
    {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        const unsigned int importFlags =
            aiProcess_LimitBoneWeights |
            aiProcess_ConvertToLeftHanded;

        // VFS를 통해 파일 로드
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        if (!vfs.LoadFile(filePath, fileData))
        {
            LOG_ERROR("FBX 파일 로드 실패: {} - FBXAssetData", filePath);
            return;
        }

        const aiScene* scene = importer.ReadFileFromMemory(
            fileData.data(),
            fileData.size(),
            importFlags,
            filePath.c_str());

        // bone 생성
        m_skeleton = std::make_shared<SkeletonData>();
        m_skeleton->Create(scene);

        // material 생성
        m_animation = std::make_shared<AnimationData>();
        m_animation->Create(scene, m_skeleton);
    }
}