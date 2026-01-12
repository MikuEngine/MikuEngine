#include "EnginePCH.h"
#include "LightSystem.h"

namespace engine
{
	Light* LightSystem::GetMainLight() const
	{
		for (auto light : m_components)
		{
			if (light->IsActive() && light->GetLightType() == LightType::Directional)
			{
				return light;
			}
		}
		return nullptr;
	}

	const std::vector<Light*>& LightSystem::GetLights() const
	{
		return m_components;
	}
}