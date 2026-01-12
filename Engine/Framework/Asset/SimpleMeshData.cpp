#include "EnginePCH.h"
#include "SimpleMeshData.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace engine
{
    void SimpleMeshData::Create(const std::string& filePath)
    {
        Assimp::Importer importer;

        const unsigned int importFlags =
            aiProcess_Triangulate |
            aiProcess_GenUVCoords |
            aiProcess_ConvertToLeftHanded |
            aiProcess_PreTransformVertices;

        const aiScene* scene = importer.ReadFile(filePath, importFlags);

        Create(scene);
    }

    void SimpleMeshData::Create(const aiScene* scene)
    {
        const aiMesh* mesh = scene->mMeshes[0];

        for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
        {
            m_vertices.emplace_back(
                &mesh->mVertices[j].x,
                &mesh->mTextureCoords[0][j].x);
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
        {
            m_indices.push_back(mesh->mFaces[j].mIndices[0]);
            m_indices.push_back(mesh->mFaces[j].mIndices[1]);
            m_indices.push_back(mesh->mFaces[j].mIndices[2]);
        }
    }

    const std::vector<PositionTexCoordVertex>& SimpleMeshData::GetVertices() const
    {
        return m_vertices;
    }

    const std::vector<WORD>& SimpleMeshData::GetIndices() const
    {
        return m_indices;
    }
}