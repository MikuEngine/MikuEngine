#include "pch.h"
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

                for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
                {
                    m_boneWeightVertices.emplace_back(
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

                for (unsigned int j = 0; j < mesh->mNumBones; ++j)
                {
                    const aiBone* bone = mesh->mBones[j];
                    const std::string boneName{ bone->mName.C_Str() };

                    const unsigned int boneIndex = skeletonData->GetBoneIndexByBoneName(boneName);

                    skeletonData->SetBoneOffset(Matrix(&bone->mOffsetMatrix.a1).Transpose(), boneIndex);

                    for (unsigned int k = 0; k < bone->mNumWeights; ++k)
                    {
                        unsigned int vertexId = bone->mWeights[k].mVertexId + m_meshSections[i].vertexOffset;
                        float weight = bone->mWeights[k].mWeight;

                        m_boneWeightVertices[vertexId].AddBoneData(boneIndex, weight);
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