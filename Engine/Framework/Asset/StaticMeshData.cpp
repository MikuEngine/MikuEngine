#include "pch.h"
#include "StaticMeshData.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace engine
{
    void StaticMeshData::Create(const std::string& filePath)
    {
        Assimp::Importer importer;

        const unsigned int importFlags = aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_GenUVCoords |
            aiProcess_CalcTangentSpace |
            aiProcess_ConvertToLeftHanded |
            aiProcess_PreTransformVertices;

        const aiScene* scene = importer.ReadFile(filePath, importFlags);

        Create(scene);
    }

    void StaticMeshData::Create(const aiScene* scene)
    {
        m_meshSections.reserve(scene->mNumMeshes);

        // vertices, indices 각각 하나의 컨테이너만 사용하고
        // offset으로 section 구분함
        int totalVertices = 0;
        unsigned int totalIndices = 0;

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[i];

            m_meshSections.emplace_back(
                mesh->mName.C_Str(),
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

    void StaticMeshData::Create(std::vector<CommonVertex>&& vertices, std::vector<DWORD>&& indices)
    {
        m_vertices = std::move(vertices);
        m_indices = std::move(indices);
        m_meshSections.push_back({ .indexCount = static_cast<UINT>(m_indices.size()) });
    }

    const std::vector<CommonVertex>& StaticMeshData::GetVertices() const
    {
        return m_vertices;
    }

    const std::vector<DWORD>& StaticMeshData::GetIndices() const
    {
        return m_indices;
    }

    const std::vector<StaticMeshSection>& StaticMeshData::GetMeshSections() const
    {
        return m_meshSections;
    }
}