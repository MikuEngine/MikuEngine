#include "EnginePCH.h"
#include "SkeletalMeshData.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Framework/Asset/SkeletonData.h"

namespace engine
{
    void SkeletalMeshData::Create(const aiScene* scene, const std::shared_ptr<SkeletonData>& skeletonData, bool isRigid)
    {
        m_isRigid = isRigid;

        m_meshSections.reserve(scene->mNumMeshes);

        int totalVertices = 0;
        unsigned int totalIndices = 0;

        if (isRigid)
        {
            for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
            {
                const aiMesh* mesh = scene->mMeshes[i];
                const std::string name{ mesh->mName.C_Str() };
                m_meshSections.emplace_back(
                    name,
                    skeletonData->GetBoneIndexByMeshName(name),
                    mesh->mMaterialIndex,
                    totalVertices,
                    totalIndices,
                    mesh->mNumFaces * 3);

                totalVertices += mesh->mNumVertices;
                totalIndices += mesh->mNumFaces * 3;
            }

            m_vertices.reserve(totalVertices);
            m_indices.reserve(totalIndices);

            for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
            {
                const aiMesh* mesh = scene->mMeshes[i];

                for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
                {
                    m_vertices.emplace_back(
                        &mesh->mVertices[j].x,
                        &mesh->mTextureCoords[0][j].x,
                        &mesh->mNormals[j].x,
                        &mesh->mTangents[j].x,
                        &mesh->mBitangents[j].x);
                }

                for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
                {
                    m_indices.push_back(mesh->mFaces[j].mIndices[0]);
                    m_indices.push_back(mesh->mFaces[j].mIndices[1]);
                    m_indices.push_back(mesh->mFaces[j].mIndices[2]);
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
            {
                const aiMesh* mesh = scene->mMeshes[i];
                m_meshSections.emplace_back(
                    mesh->mName.C_Str(),
                    0,
                    mesh->mMaterialIndex,
                    totalVertices,
                    totalIndices,
                    mesh->mNumFaces * 3);

                totalVertices += mesh->mNumVertices;
                totalIndices += mesh->mNumFaces * 3;
            }

            m_boneWeightVertices.reserve(totalVertices);
            m_indices.reserve(totalIndices);

            for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
            {
                const aiMesh* mesh = scene->mMeshes[i];

                if (mesh->mTextureCoords[0] == nullptr)
                {
                    for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
                    {
                        static constexpr float v2[2]{ 0.0f, 0.0f };
                        static constexpr float v3[3]{ 0.0f, 0.0f, 0.0f };

                        m_boneWeightVertices.emplace_back(
                            &mesh->mVertices[j].x,
                            &v2[0],
                            &mesh->mNormals[j].x,
                            &v3[0],
                            &v3[0]);
                    }
                }
                else
                {
                    for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
                    {
                        m_boneWeightVertices.emplace_back(
                            &mesh->mVertices[j].x,
                            &mesh->mTextureCoords[0][j].x,
                            &mesh->mNormals[j].x,
                            &mesh->mTangents[j].x,
                            &mesh->mBitangents[j].x);
                    }
                }

                for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
                {
                    m_indices.push_back(mesh->mFaces[j].mIndices[0]);
                    m_indices.push_back(mesh->mFaces[j].mIndices[1]);
                    m_indices.push_back(mesh->mFaces[j].mIndices[2]);
                }

                if (mesh->mNumBones > 0)
                {
                    // 1. 기존 로직: 본이 있는 경우 정상적으로 가중치 할당
                    for (unsigned int j = 0; j < mesh->mNumBones; ++j)
                    {
                        const aiBone* bone = mesh->mBones[j];
                        const std::string boneName{ bone->mName.C_Str() };

                        const unsigned int boneIndex = skeletonData->GetBoneIndexByBoneName(boneName);

                        skeletonData->SetBoneOffset(Matrix(&bone->mOffsetMatrix.a1).Transpose(), boneIndex);

                        for (unsigned int k = 0; k < bone->mNumWeights; ++k)
                        {
                            // vertexOffset은 m_meshSections[i]에서 가져와야 하는데, 
                            // 현재 코드 문맥상 루프 바깥 i 인덱스를 참조해야 합니다.
                            unsigned int vertexId = bone->mWeights[k].mVertexId + m_meshSections[i].vertexOffset;
                            float weight = bone->mWeights[k].mWeight;

                            m_boneWeightVertices[vertexId].AddBoneData(boneIndex, weight);
                        }
                    }
                }
                else
                {
                    // 2. 추가된 로직: 본이 없는 정적 메쉬(test_3 등) 처리
                    // 강제로 루트 본(보통 0번 인덱스)에 모든 버텍스를 종속시킵니다.

                    // 주의: 0번이 루트가 아니라면 skeletonData->GetBoneIndexByBoneName("RootName") 등을 사용해야 함
                    unsigned int rootBoneIndex = 0;
                    unsigned int startVertexOffset = m_meshSections[i].vertexOffset;

                    for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        unsigned int vertexId = startVertexOffset + v;

                        // 해당 버텍스를 0번 본(루트)에 100%(1.0f) 가중치로 할당
                        // 이렇게 하면 캐릭터의 루트 움직임(이동/회전)을 그대로 따라다니게 됩니다.
                        m_boneWeightVertices[vertexId].AddBoneData(rootBoneIndex, 1.0f);
                    }
                }
            }
        }
    }

    const std::vector<BoneWeightVertex>& SkeletalMeshData::GetBoneWeightVertices() const
    {
        return m_boneWeightVertices;
    }

    const std::vector<CommonVertex>& SkeletalMeshData::GetVertices() const
    {
        return m_vertices;
    }

    const std::vector<DWORD>& SkeletalMeshData::GetIndices() const
    {
        return m_indices;
    }

    const std::vector<SkeletalMeshSection>& SkeletalMeshData::GetMeshSections() const
    {
        return m_meshSections;
    }

    bool SkeletalMeshData::IsRigid() const
    {
        return m_isRigid;
    }
}