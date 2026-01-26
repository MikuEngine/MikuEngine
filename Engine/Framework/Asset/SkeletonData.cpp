#include "EnginePCH.h"
#include "SkeletonData.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <queue>

#include "Core/System/VirtualFileSystem.h"

#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    namespace
    {
        size_t GetNodeCount(const aiNode* node)
        {
            size_t count = 1;

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
            {
                count += GetNodeCount(node->mChildren[i]);
            }

            return count;
        }
    }

    void SkeletonData::Create(const std::string& filePath)
    {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        unsigned int importFlags =
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
            LOG_ERROR("Skeleton 파일 로드 실패: {} - SkeletonData", filePath);
            return;
        }

        const aiScene* scene = importer.ReadFileFromMemory(
            fileData.data(),
            fileData.size(),
            importFlags,
            filePath.c_str());

        if (!scene)
        {
            LOG_ERROR("Skeleton 파싱 실패: {} - SkeletonData", filePath);
            return;
        }

        Create(scene);
    }

    void SkeletonData::Create(const aiScene* scene)
    {
        const aiNode* root = scene->mRootNode->mChildren[0];

        const size_t nodeCount = GetNodeCount(root);
        m_bones.reserve(nodeCount);

        std::queue<std::pair<const aiNode*, int>> nodeQueue; // 상위 노드 순으로 트리 만들기 위한 큐
        nodeQueue.push({ root, -1 });

        while (!nodeQueue.empty())
        {
            const auto& [node, parentIndex] = nodeQueue.front();
            const std::string name{ node->mName.C_Str() };
            const BoneIndex index = static_cast<unsigned int>(m_bones.size());

            m_bones.emplace_back(name, Matrix(&node->mTransformation.a1).Transpose(), index, parentIndex);
            m_boneMappingTable[name] = index;

            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
            {
                const std::string meshName{ scene->mMeshes[node->mMeshes[i]]->mName.C_Str() };

                m_meshMappingTable[meshName] = index;
            }

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
            {
                nodeQueue.push({ node->mChildren[i] , index });
            }

            nodeQueue.pop();
        }
    }

    const std::vector<BoneInfo>& SkeletonData::GetBones() const
    {
        return m_bones;
    }

    unsigned int SkeletonData::GetBoneIndexByBoneName(const std::string& boneName) const
    {
        auto find = m_boneMappingTable.find(boneName);

        assert(find != m_boneMappingTable.end());

        return find->second;
    }

    unsigned int SkeletonData::GetBoneIndexByMeshName(const std::string& meshName) const
    {
        auto find = m_meshMappingTable.find(meshName);

        assert(find != m_meshMappingTable.end());

        return find->second;
    }

    BoneInfo* SkeletonData::GetBoneInfoByIndex(size_t index)
    {
        assert(index < m_bones.size());

        return &m_bones[index];
    }

    const BoneMatrixArray& SkeletonData::GetBoneOffsets() const
    {
        return m_boneOffsets;
    }

    void SkeletonData::SetBoneOffset(const Matrix& offset, unsigned int boneIndex)
    {
        assert(boneIndex < m_boneOffsets.size());

        m_boneOffsets[boneIndex] = offset;
    }

    void SkeletonData::SetupSkeletonInstance(std::vector<Bone>& out) const
    {
        out.reserve(m_bones.size());

        for (const auto& bone : m_bones)
        {
            out.emplace_back(bone.name, bone.parentIndex, bone.index, bone.relative);
        }
    }
}