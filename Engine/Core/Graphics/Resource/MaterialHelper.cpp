#include "pch.h"
#include "MaterialHelper.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Framework/Asset/MaterialData.h"

namespace engine
{
    void SetupTextures(const std::shared_ptr<MaterialData>& data, std::vector<Textures>& out)
    {
		out.reserve(data->GetMaterials().size());

		for (const auto& material : data->GetMaterials())
		{
			Textures textures{};

			if (material.materialFlags & static_cast<std::uint64_t>(MaterialKey::BASE_COLOR_TEXTURE))
			{
				textures.baseColor = ResourceManager::Get().GetOrCreateTexture(material.texturePaths.at(MaterialKey::BASE_COLOR_TEXTURE));
			}
			else
			{
				textures.baseColor = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			}

			if (material.materialFlags & static_cast<std::uint64_t>(MaterialKey::NORMAL_TEXTURE))
			{
				textures.normal = ResourceManager::Get().GetOrCreateTexture(material.texturePaths.at(MaterialKey::NORMAL_TEXTURE));
			}
			else
			{
				textures.normal = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::Normal);
			}

			if (material.materialFlags & static_cast<std::uint64_t>(MaterialKey::EMISSIVE_TEXTURE))
			{
				textures.emissive = ResourceManager::Get().GetOrCreateTexture(material.texturePaths.at(MaterialKey::EMISSIVE_TEXTURE));
			}
			else
			{
				textures.emissive = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			}

			if (material.materialFlags & static_cast<std::uint64_t>(MaterialKey::METALNESS_TEXTURE))
			{
				textures.metalness = ResourceManager::Get().GetOrCreateTexture(material.texturePaths.at(MaterialKey::METALNESS_TEXTURE));
			}
			else
			{
				textures.metalness = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::Black);
			}

			if (material.materialFlags & static_cast<std::uint64_t>(MaterialKey::ROUGHNESS_TEXTURE))
			{
				textures.roughness = ResourceManager::Get().GetOrCreateTexture(material.texturePaths.at(MaterialKey::ROUGHNESS_TEXTURE));
			}
			else
			{
				textures.roughness = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			}

			if (material.materialFlags & static_cast<std::uint64_t>(MaterialKey::AMBIENT_OCCLUSION_TEXTURE))
			{
				textures.ambientOcclusion = ResourceManager::Get().GetOrCreateTexture(material.texturePaths.at(MaterialKey::AMBIENT_OCCLUSION_TEXTURE));
			}
			else
			{
				textures.ambientOcclusion = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			}

			out.push_back(std::move(textures));
		}
    }
}