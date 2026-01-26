#include "EnginePCH.h"
#include "EnvironmentSettings.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/EnvironmentSystem.h"

namespace engine
{
    EnvironmentSettings::~EnvironmentSettings()
    {
        SystemManager::Get().GetEnvironmentSystem().Unregister(this);
    }

    void EnvironmentSettings::Initialize()
    {
        SystemManager::Get().GetEnvironmentSystem().Register(this);
    }

    void EnvironmentSettings::SetSkyboxTexturePath(const std::string& path)
    {
        m_skyboxTexturePath = path;
    }

    void EnvironmentSettings::SetIBLIrradiancePath(const std::string& path)
    {
        m_iblIrradiancePath = path;
    }

    void EnvironmentSettings::SetIBLSpecularPath(const std::string& path)
    {
        m_iblSpecularPath = path;
    }

    void EnvironmentSettings::SetIBLBrdfLutPath(const std::string& path)
    {
        m_iblBrdfLutPath = path;
    }

    const std::string& EnvironmentSettings::GetSkyboxTexturePath() const
    {
        return m_skyboxTexturePath;
    }

    const std::string& EnvironmentSettings::GetIBLIrradiancePath() const
    {
        return m_iblIrradiancePath;
    }

    const std::string& EnvironmentSettings::GetIBLSpecularPath() const
    {
        return m_iblSpecularPath;
    }

    const std::string& EnvironmentSettings::GetIBLBrdfLutPath() const
    {
        return m_iblBrdfLutPath;
    }

    void EnvironmentSettings::SetSkyboxColor(const Vector3& color)
    {
        m_skyboxColor = color;
    }

    void EnvironmentSettings::SetSkyboxHorizonColor(const Vector3& color)
    {
        m_skyboxHorizonColor = color;
    }

    void EnvironmentSettings::SetIBLAmbientColor(const Vector3& color)
    {
        m_iblAmbientColor = color;
    }

    const Vector3& EnvironmentSettings::GetSkyboxColor() const
    {
        return m_skyboxColor;
    }

    const Vector3& EnvironmentSettings::GetSkyboxHorizonColor() const
    {
        return m_skyboxHorizonColor;
    }

    const Vector3& EnvironmentSettings::GetIBLAmbientColor() const
    {
        return m_iblAmbientColor;
    }

    void EnvironmentSettings::SetUseSkyboxTexture(int use)
    {
        m_useSkyboxTexture = use;
    }

    void EnvironmentSettings::SetUseIBLTexture(int use)
    {
        m_useIBLTexture = use;
    }

    void EnvironmentSettings::SetUseIBL(int use)
    {
        m_useIBL = use;
    }

    int EnvironmentSettings::GetUseSkyboxTexture() const
    {
        return m_useSkyboxTexture;
    }

    int EnvironmentSettings::GetUseIBLTexture() const
    {
        return m_useIBLTexture;
    }

    int EnvironmentSettings::GetUseIBL() const
    {
        return m_useIBL;
    }

    bool EnvironmentSettings::HasSkyboxColorOverride() const
    {
        return m_skyboxColor.x >= 0.0f && m_skyboxColor.y >= 0.0f && m_skyboxColor.z >= 0.0f;  // 기본값이 음수면 오버라이드 없음
    }

    bool EnvironmentSettings::HasSkyboxHorizonColorOverride() const
    {
        return m_skyboxHorizonColor.x >= 0.0f && m_skyboxHorizonColor.y >= 0.0f && m_skyboxHorizonColor.z >= 0.0f;
    }

    bool EnvironmentSettings::HasIBLAmbientColorOverride() const
    {
        return m_iblAmbientColor.x >= 0.0f && m_iblAmbientColor.y >= 0.0f && m_iblAmbientColor.z >= 0.0f;
    }

    void EnvironmentSettings::OnGui()
    {
        // 텍스처 경로
        ImGui::SeparatorText("Texture Paths");
        
        // Skybox Texture
        ImGui::Text("Skybox Texture: %s", m_skyboxTexturePath.empty() ? "None" : std::filesystem::path(m_skyboxTexturePath).filename().string().c_str());
        std::string selectedSkybox;
        static std::vector<std::string> ddsExtensions{ ".dds" };
        if (DrawFileSelector("Select Skybox Texture", "Resource/Texture", ddsExtensions, selectedSkybox))
        {
            m_skyboxTexturePath = selectedSkybox;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##Skybox"))
        {
            m_skyboxTexturePath.clear();
        }

        // IBL Irradiance
        ImGui::Text("IBL Irradiance: %s", m_iblIrradiancePath.empty() ? "None" : std::filesystem::path(m_iblIrradiancePath).filename().string().c_str());
        std::string selectedIrradiance;
        if (DrawFileSelector("Select IBL Irradiance", "Resource/Texture", ddsExtensions, selectedIrradiance))
        {
            m_iblIrradiancePath = selectedIrradiance;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##Irradiance"))
        {
            m_iblIrradiancePath.clear();
        }

        // IBL Specular
        ImGui::Text("IBL Specular: %s", m_iblSpecularPath.empty() ? "None" : std::filesystem::path(m_iblSpecularPath).filename().string().c_str());
        std::string selectedSpecular;
        if (DrawFileSelector("Select IBL Specular", "Resource/Texture", ddsExtensions, selectedSpecular))
        {
            m_iblSpecularPath = selectedSpecular;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##Specular"))
        {
            m_iblSpecularPath.clear();
        }

        // IBL BRDF LUT
        ImGui::Text("IBL BRDF LUT: %s", m_iblBrdfLutPath.empty() ? "None" : std::filesystem::path(m_iblBrdfLutPath).filename().string().c_str());
        std::string selectedBrdfLut;
        if (DrawFileSelector("Select IBL BRDF LUT", "Resource/Texture", ddsExtensions, selectedBrdfLut))
        {
            m_iblBrdfLutPath = selectedBrdfLut;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##BrdfLut"))
        {
            m_iblBrdfLutPath.clear();
        }

        // 색상
        ImGui::SeparatorText("Colors");
        bool overrideSkyboxColor = HasSkyboxColorOverride();
        if (ImGui::Checkbox("Override Skybox Color", &overrideSkyboxColor))
        {
            if (overrideSkyboxColor && m_skyboxColor.x < 0.0f)
                m_skyboxColor = { 0.5f, 0.7f, 1.0f };  // 기본값
            else if (!overrideSkyboxColor)
                m_skyboxColor = { -1.0f, -1.0f, -1.0f };
        }
        if (overrideSkyboxColor)
        {
            ImGui::ColorEdit3("Skybox Color", &m_skyboxColor.x);
        }

        bool overrideSkyboxHorizonColor = HasSkyboxHorizonColorOverride();
        if (ImGui::Checkbox("Override Skybox Horizon Color", &overrideSkyboxHorizonColor))
        {
            if (overrideSkyboxHorizonColor && m_skyboxHorizonColor.x < 0.0f)
                m_skyboxHorizonColor = { 0.3f, 0.4f, 0.5f };  // 기본값
            else if (!overrideSkyboxHorizonColor)
                m_skyboxHorizonColor = { -1.0f, -1.0f, -1.0f };
        }
        if (overrideSkyboxHorizonColor)
        {
            ImGui::ColorEdit3("Skybox Horizon Color", &m_skyboxHorizonColor.x);
        }

        bool overrideIBLAmbientColor = HasIBLAmbientColorOverride();
        if (ImGui::Checkbox("Override IBL Ambient Color", &overrideIBLAmbientColor))
        {
            if (overrideIBLAmbientColor && m_iblAmbientColor.x < 0.0f)
                m_iblAmbientColor = { 0.5f, 0.7f, 1.0f };  // 기본값
            else if (!overrideIBLAmbientColor)
                m_iblAmbientColor = { -1.0f, -1.0f, -1.0f };
        }
        if (overrideIBLAmbientColor)
        {
            ImGui::ColorEdit3("IBL Ambient Color", &m_iblAmbientColor.x);
        }

        // 플래그
        ImGui::SeparatorText("Flags");
        int useSkyboxTex = m_useSkyboxTexture + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
        if (ImGui::Combo("Use Skybox Texture", &useSkyboxTex, "Use ProjectSettings\0False\0True\0", 3))
        {
            m_useSkyboxTexture = useSkyboxTex - 1;  // 0 -> -1, 1 -> 0, 2 -> 1
        }

        int useIBLTex = m_useIBLTexture + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
        if (ImGui::Combo("Use IBL Texture", &useIBLTex, "Use ProjectSettings\0False\0True\0", 3))
        {
            m_useIBLTexture = useIBLTex - 1;  // 0 -> -1, 1 -> 0, 2 -> 1
        }

        int useIBL = m_useIBL + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
        if (ImGui::Combo("Use IBL", &useIBL, "Use ProjectSettings\0False\0True\0", 3))
        {
            m_useIBL = useIBL - 1;  // 0 -> -1, 1 -> 0, 2 -> 1
        }
    }

    void EnvironmentSettings::Save(json& j) const
    {
        Object::Save(j);

        j["SkyboxTexturePath"] = m_skyboxTexturePath;
        j["IBLIrradiancePath"] = m_iblIrradiancePath;
        j["IBLSpecularPath"] = m_iblSpecularPath;
        j["IBLBrdfLutPath"] = m_iblBrdfLutPath;
        j["SkyboxColor"] = m_skyboxColor;
        j["SkyboxHorizonColor"] = m_skyboxHorizonColor;
        j["IBLAmbientColor"] = m_iblAmbientColor;
        j["UseSkyboxTexture"] = m_useSkyboxTexture;
        j["UseIBLTexture"] = m_useIBLTexture;
        j["UseIBL"] = m_useIBL;
    }

    void EnvironmentSettings::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j, "SkyboxTexturePath", m_skyboxTexturePath);
        JsonGet(j, "IBLIrradiancePath", m_iblIrradiancePath);
        JsonGet(j, "IBLSpecularPath", m_iblSpecularPath);
        JsonGet(j, "IBLBrdfLutPath", m_iblBrdfLutPath);
        JsonGet(j, "SkyboxColor", m_skyboxColor);
        JsonGet(j, "SkyboxHorizonColor", m_skyboxHorizonColor);
        JsonGet(j, "IBLAmbientColor", m_iblAmbientColor);
        JsonGet(j, "UseSkyboxTexture", m_useSkyboxTexture);
        JsonGet(j, "UseIBLTexture", m_useIBLTexture);
        JsonGet(j, "UseIBL", m_useIBL);
    }
}
