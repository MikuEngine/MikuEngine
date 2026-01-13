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

	void Light::SetAngle(float angle)
	{
		m_angle = angle;
	}

	void Light::SetLightNear(float lightNear)
	{
		m_lightNear = lightNear;
	}

	void Light::SetLightFar(float lightFar)
	{
		m_lightFar = lightFar;
	}

	void Light::SetForwardDist(float forwardDist)
	{
		m_lightForwardDist = forwardDist;
	}

	void Light::SetHeightRatio(float heightRatio)
	{
		m_lightHeightRatio = heightRatio;
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

	float Light::GetAngle() const
	{
		return m_angle;
	}

	float Light::GetLightNear() const
	{
		return m_lightNear;
	}

	float Light::GetLightFar() const
	{
		return m_lightFar;
	}

	float Light::GetForwardDist() const
	{
		return m_lightForwardDist;
	}

	float Light::GetHeightRatio() const
	{
		return m_lightHeightRatio;
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
		ImGui::DragFloat("Intensity", &m_intensity, 0.1f, 0.0f, 100.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
		// Type Specific Properties
		if (m_lightType == LightType::Directional)
		{
			ImGui::SeparatorText("Shadow Frustum Setting");
			ImGui::DragFloat("Near", &m_lightNear, 1.0f, 1.0f, m_lightFar - 10.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Far", &m_lightFar, 1.0f, m_lightNear + 10.0f, FLT_MAX, "%.0f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("FOV", &m_angle, 0.1f, 0.1f, 189.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Forward Distance", &m_lightForwardDist);
			ImGui::DragFloat("Height Ratio", &m_lightHeightRatio, 0.01f, 0.01f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		}

		if (m_lightType == LightType::Point || m_lightType == LightType::Spot)
		{
			ImGui::DragFloat("Range", &m_range, 0.1f, 0.0f, FLT_MAX, "%.1f", ImGuiSliderFlags_AlwaysClamp);
		}
		if (m_lightType == LightType::Spot)
		{
			ImGui::SliderFloat("Spot Angle", &m_angle, 0.1f, 189.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
		}
	}

	void Light::Save(json& j) const
	{
		Object::Save(j);

		j["LightType"] = m_lightType;
		j["Color"] = m_color;
		j["Intensity"] = m_intensity;
		j["Range"] = m_range;
		j["Angle"] = m_angle;
		j["Near"] = m_lightNear;
		j["Far"] = m_lightFar;
		j["ForwardDistance"] = m_lightForwardDist;
		j["HeightRatio"] = m_lightHeightRatio;
	}

	void Light::Load(const json& j)
	{
		Object::Load(j);

		JsonGet(j, "LightType", m_lightType);
		JsonGet(j, "Color", m_color);
		JsonGet(j, "Intensity", m_intensity);
		JsonGet(j, "Range", m_range);
		JsonGet(j, "Angle", m_angle);
		JsonGet(j, "Near", m_lightNear);
		JsonGet(j, "Far", m_lightFar);
		JsonGet(j, "ForwardDistance", m_lightForwardDist);
		JsonGet(j, "HeightRatio", m_lightHeightRatio);
	}

	std::string Light::GetType() const
	{
		return "Light";
	}
}