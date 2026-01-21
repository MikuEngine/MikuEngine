#include "EnginePCH.h"
#include "MaterialData.h"

#include <filesystem>
#include <string_view>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace engine
{
    namespace
    {
        std::filesystem::path g_basePath{ "Resource/Model" };
    }

    void MaterialData::Create()
    {
        m_materials.push_back({ .materialFlags = 0 });
    }

    void MaterialData::Create(const std::string& filePath)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(filePath, 0);

        if (!scene)
        {
            LOG_ERROR("[MaterialData] Failed to load material: {}", filePath);
            return;
        }

        Create(scene);
    }

    void MaterialData::Create(const aiScene* scene)
    {
        namespace fs = std::filesystem;

        if (!scene)
        {
            LOG_ERROR("[MaterialData] scene is null");
            return;
        }

        aiString path;
        aiColor4D color;
        float scalar = 0.0f;

        m_materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            const aiMaterial* aiMaterial = scene->mMaterials[i];

            Material material{};

            std::string_view materialName = aiMaterial->GetName().C_Str();

            // 기본 Opaque
            if (materialName.ends_with("_Cutout"))
            {
                material.renderType = MaterialRenderType::Cutout;
            }
            else if (materialName.ends_with("_Alpha"))
            {
                material.renderType = MaterialRenderType::Transparent;
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path))
            {
                material.texturePaths[MaterialKey::BASE_COLOR_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::BASE_COLOR_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path))
            {
                material.texturePaths[MaterialKey::NORMAL_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::NORMAL_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path))
            {
                material.texturePaths[MaterialKey::EMISSIVE_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::EMISSIVE_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &path))
            {
                material.texturePaths[MaterialKey::METALNESS_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::METALNESS_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path))
            {
                material.texturePaths[MaterialKey::ROUGHNESS_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE);
            }
            else if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &path))
            {
                material.texturePaths[MaterialKey::ROUGHNESS_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path))
            {
                material.texturePaths[MaterialKey::AMBIENT_OCCLUSION_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE);
            }
            else if (aiReturn_SUCCESS == aiMaterial->Get("$raw.AmbientOcclusionTexture", 0, 0, path))
            {
                material.texturePaths[MaterialKey::AMBIENT_OCCLUSION_TEXTURE] = (g_basePath / fs::path(path.C_Str()).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE);
            }

            m_materials.push_back(std::move(material));
        }
    }
    
    const std::vector<Material>& MaterialData::GetMaterials() const
    {
        return m_materials;
    }
}