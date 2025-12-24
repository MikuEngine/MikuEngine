#include "pch.h"
#include "MaterialData.h"

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace engine
{
    void MaterialData::Create()
    {
        m_materials.push_back({ .materialFlags = 0 });
    }

    void MaterialData::Create(const std::string& filePath)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(filePath, 0);

        Create(scene);
    }

    void MaterialData::Create(const aiScene* scene)
    {
        namespace fs = std::filesystem;

        aiString path;
        aiColor4D color;
        float scalar = 0.0f;

        m_materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            const aiMaterial* aiMaterial = scene->mMaterials[i];

            Material material{};

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path))
            {
                material.texturePaths[MaterialKey::BASE_COLOR_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::BASE_COLOR_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path))
            {
                material.texturePaths[MaterialKey::NORMAL_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::NORMAL_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path))
            {
                material.texturePaths[MaterialKey::EMISSIVE_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::EMISSIVE_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &path))
            {
                material.texturePaths[MaterialKey::METALNESS_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::METALNESS_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path))
            {
                material.texturePaths[MaterialKey::ROUGHNESS_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE);
            }
            else if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &path))
            {
                material.texturePaths[MaterialKey::ROUGHNESS_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path))
            {
                material.texturePaths[MaterialKey::AMBIENT_OCCLUSION_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE);
            }
            else if (aiReturn_SUCCESS == aiMaterial->Get("$raw.AmbientOcclusionTexture", 0, 0, path))
            {
                material.texturePaths[MaterialKey::AMBIENT_OCCLUSION_TEXTURE] = (fs::path("Resource/Model") / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
            {
                material.vectorValues[MaterialKey::BASE_COLOR] = { color.r, color.g, color.b, color.a };
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::BASE_COLOR);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color))
            {
                material.vectorValues[MaterialKey::EMISSIVE_COLOR] = { color.r, color.g, color.b, 1.0f };
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::EMISSIVE_COLOR);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get("$raw.Roughness", 0, 0, scalar))
            {
                material.scalarValues[MaterialKey::ROUGHNESS] = scalar;
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get("$raw.Metalness", 0, 0, scalar))
            {
                material.scalarValues[MaterialKey::METALNESS] = scalar;
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::METALNESS);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get("$raw.EmissiveIntensity", 0, 0, scalar))
            {
                material.scalarValues[MaterialKey::EMISSIVE_INTENSITY] = scalar;
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::EMISSIVE_INTENSITY);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get("$raw.AmbientOcclusion", 0, 0, scalar))
            {
                material.scalarValues[MaterialKey::AMBIENT_OCCLUSION] = scalar;
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION);
            }

            m_materials.push_back(std::move(material));
        }
    }
    
    const std::vector<Material>& MaterialData::GetMaterials() const
    {
        return m_materials;
    }
}