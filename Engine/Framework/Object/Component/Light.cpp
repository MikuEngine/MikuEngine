#include "EnginePCH.h"
#include "Light.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/LightSystem.h"

void to_json(nlohmann::ordered_json& j, engine::LightType type)
{
	j = nlohmann::ordered_json{ "LightType", static_cast<int>(type) };
}

void from_json(const nlohmann::ordered_json& j, engine::LightType& type)
{
	type = static_cast<engine::LightType>(j.at("LightType"));
}

namespace engine
{
	Light::~Light()
	{
		SystemManager::Get().GetLightSystem().Unregister(this);
	}

	void Light::Initialize()
	{
		SystemManager::Get().GetLightSystem().Register(this);
	}

	void Light::SetLightType(LightType lightType)
	{
		m_lightType = lightType;
	}

	void Light::SetColor(const Vector3& color)
	{
		m_color = color;
	}

	void Light::SetIntensity(float intensity)
	{
		m_intensity = intensity;
	}

	void Light::SetRange(float range)
	{
		m_range = range;
	}

	void Light::SetSpotAngle(float angle)
	{
		m_spotAngle = angle;
	}

	LightType Light::GetLightType() const
	{
		return m_lightType;
	}

	const Vector3& Light::GetColor() const
	{
		return m_color;
	}

	float Light::GetIntensity() const
	{
		return m_intensity;
	}

	float Light::GetRange() const
	{
		return m_range;
	}

	float Light::GetSpotAngle() const
	{
		return m_spotAngle;
	}

	void Light::OnGui()
	{
		// Light Type
		static const char* lightTypes[] = { "Directional", "Point", "Spot" };
		int currentType = static_cast<int>(m_lightType);
		if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
		{
			m_lightType = static_cast<LightType>(currentType);
		}
		// Common Properties
		ImGui::ColorEdit3("Color", &m_color.x);
		ImGui::DragFloat("Intensity", &m_intensity, 0.1f, 0.0f, 100.0f);
		// Type Specific Properties
		if (m_lightType == LightType::Point || m_lightType == LightType::Spot)
		{
			ImGui::DragFloat("Range", &m_range, 0.1f, 0.0f, 100.0f);
		}
		if (m_lightType == LightType::Spot)
		{
			ImGui::SliderFloat("Spot Angle", &m_spotAngle, 1.0f, 90.0f);
		}
	}

	void Light::Save(json& j) const
	{
		Object::Save(j);

		j["LightType"] = m_lightType;
		j["Color"] = m_color;
		j["Intensity"] = m_intensity;
		j["Range"] = m_range;
		j["SpotAngle"] = m_spotAngle;
	}

	void Light::Load(const json& j)
	{
		Object::Load(j);

		JsonGet(j, "LightType", m_lightType);
		JsonGet(j, "Color", m_color);
		JsonGet(j, "Intensity", m_intensity);
		JsonGet(j, "Range", m_range);
		JsonGet(j, "SpotAngle", m_spotAngle);
	}

	std::string Light::GetType() const
	{
		return "Light";
	}
}