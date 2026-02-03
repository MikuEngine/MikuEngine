#include "EnginePCH.h"
#include "MaterialData.h"

#include <filesystem>
#include <string_view>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    namespace
    {
        constexpr std::string_view g_basePathPrefix{ "Resource/Model" };
    }

    void MaterialData::Create()
    {
        m_materials.push_back({ .materialFlags = 0 });
    }

    void MaterialData::Create(const std::string& filePath)
    {
        Assimp::Importer importer;

        // VFS를 통해 파일 로드
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        
        if (!vfs.LoadFile(filePath, fileData))
        {
            LOG_ERROR("[MaterialData] Failed to load material: {}", filePath);
            return;
        }

        const aiScene* scene = importer.ReadFileFromMemory(
            fileData.data(),
            fileData.size(),
            0,
            filePath.c_str());

        if (!scene)
        {
            LOG_ERROR("[MaterialData] Failed to parse material: {}", filePath);
            return;
        }

        Create(scene, filePath);
    }

    void MaterialData::Create(const aiScene* scene, const std::string& filePath)
    {
        namespace fs = std::filesystem;

        if (!scene)
        {
            LOG_ERROR("[MaterialData] scene is null");
            return;
        }

        // filePath에서 Resource/Model/ 다음 부분 추출하여 텍스처 기본 경로 생성
        fs::path textureBasePath{ g_basePathPrefix };
        if (!filePath.empty())
        {
            fs::path filePathObj{ filePath };
            filePathObj = filePathObj.lexically_normal();
            
            std::string filePathStr = filePathObj.generic_string(); // '/' 구분자로 통일
            std::string basePathStr{ g_basePathPrefix };
            
            // Resource/Model/ 위치 찾기
            size_t modelPos = filePathStr.find(basePathStr);
            if (modelPos != std::string::npos)
            {
                // Resource/Model/ 다음 부분 추출
                size_t startPos = modelPos + basePathStr.length();
                if (startPos < filePathStr.length() && filePathStr[startPos] == '/')
                {
                    startPos++; // '/' 건너뛰기
                }
                
                // 파일명 제거하고 디렉토리 경로만 가져오기
                if (startPos < filePathStr.length())
                {
                    std::string subPath = filePathStr.substr(startPos);
                    fs::path fullPath{ subPath };
                    fs::path parentPath = fullPath.parent_path();
                    
                    if (!parentPath.empty())
                    {
                        textureBasePath = fs::path{ g_basePathPrefix } / parentPath;
                        textureBasePath = textureBasePath.lexically_normal();
                    }
                }
            }
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
                material.texturePaths[MaterialKey::BASE_COLOR_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::BASE_COLOR_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path))
            {
                material.texturePaths[MaterialKey::NORMAL_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::NORMAL_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path))
            {
                material.texturePaths[MaterialKey::EMISSIVE_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::EMISSIVE_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &path))
            {
                material.texturePaths[MaterialKey::METALNESS_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::METALNESS_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path))
            {
                material.texturePaths[MaterialKey::ROUGHNESS_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE);
            }
            else if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &path))
            {
                material.texturePaths[MaterialKey::ROUGHNESS_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path))
            {
                material.texturePaths[MaterialKey::AMBIENT_OCCLUSION_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE);
            }
            else if (aiReturn_SUCCESS == aiMaterial->Get("$raw.AmbientOcclusionTexture", 0, 0, path))
            {
                material.texturePaths[MaterialKey::AMBIENT_OCCLUSION_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE);
            }

            if (aiReturn_SUCCESS == aiMaterial->Get("$raw.ThicknessTexture", 0, 0, path))
            {
                material.texturePaths[MaterialKey::THICKNESS_TEXTURE] = (textureBasePath / fs::path(ToWideChar(path.C_Str())).filename()).string();
                material.materialFlags |= static_cast<std::uint64_t>(MaterialKey::THICKNESS_TEXTURE);
            }

            m_materials.push_back(std::move(material));
        }
    }
    
    const std::vector<Material>& MaterialData::GetMaterials() const
    {
        return m_materials;
    }
}