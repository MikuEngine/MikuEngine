#include "EnginePCH.h"
#include "PostProcessingSettings.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/PostProcessingSystem.h"

namespace engine
{
    PostProcessingSettings::~PostProcessingSettings()
    {
        SystemManager::Get().GetPostProcessingSystem().Unregister(this);
    }

    void PostProcessingSettings::Initialize()
    {
        SystemManager::Get().GetPostProcessingSystem().Register(this);
    }

    void PostProcessingSettings::SetBloomStrength(float strength)
    {
        m_bloomStrength = strength;
    }

    void PostProcessingSettings::SetBloomThreshold(float threshold)
    {
        m_bloomThreshold = threshold;
    }

    void PostProcessingSettings::SetBloomSoftKnee(float softKnee)
    {
        m_bloomSoftKnee = softKnee;
    }

    void PostProcessingSettings::SetEnableBloom(int enable)
    {
        m_enableBloom = enable;
    }

    float PostProcessingSettings::GetBloomStrength() const
    {
        return m_bloomStrength;
    }

    float PostProcessingSettings::GetBloomThreshold() const
    {
        return m_bloomThreshold;
    }

    float PostProcessingSettings::GetBloomSoftKnee() const
    {
        return m_bloomSoftKnee;
    }

    int PostProcessingSettings::GetEnableBloom() const
    {
        return m_enableBloom;
    }

    void PostProcessingSettings::SetExposure(float exposure)
    {
        m_exposure = exposure;
    }

    void PostProcessingSettings::SetEnableToneMapping(int enable)
    {
        m_enableToneMapping = enable;
    }

    float PostProcessingSettings::GetExposure() const
    {
        return m_exposure;
    }

    int PostProcessingSettings::GetEnableToneMapping() const
    {
        return m_enableToneMapping;
    }

    void PostProcessingSettings::SetFXAAQualitySubpix(float value)
    {
        m_fxaaQualitySubpix = value;
    }

    void PostProcessingSettings::SetFXAAQualityEdgeThreshold(float value)
    {
        m_fxaaQualityEdgeThreshold = value;
    }

    void PostProcessingSettings::SetFXAAQualityEdgeThresholdMin(float value)
    {
        m_fxaaQualityEdgeThresholdMin = value;
    }

    void PostProcessingSettings::SetEnableFXAA(int enable)
    {
        m_enableFXAA = enable;
    }

    float PostProcessingSettings::GetFXAAQualitySubpix() const
    {
        return m_fxaaQualitySubpix;
    }

    float PostProcessingSettings::GetFXAAQualityEdgeThreshold() const
    {
        return m_fxaaQualityEdgeThreshold;
    }

    float PostProcessingSettings::GetFXAAQualityEdgeThresholdMin() const
    {
        return m_fxaaQualityEdgeThresholdMin;
    }

    int PostProcessingSettings::GetEnableFXAA() const
    {
        return m_enableFXAA;
    }

    bool PostProcessingSettings::HasBloomStrengthOverride() const
    {
        return m_bloomStrength >= 0.0f;
    }

    bool PostProcessingSettings::HasBloomThresholdOverride() const
    {
        return m_bloomThreshold >= 0.0f;
    }

    bool PostProcessingSettings::HasBloomSoftKneeOverride() const
    {
        return m_bloomSoftKnee >= 0.0f;
    }

    bool PostProcessingSettings::HasExposureOverride() const
    {
        return m_exposure >= -1000.0f;  // exposure는 음수일 수 있으므로 매우 작은 값으로 체크
    }

    bool PostProcessingSettings::HasFXAAQualitySubpixOverride() const
    {
        return m_fxaaQualitySubpix >= 0.0f;
    }

    bool PostProcessingSettings::HasFXAAQualityEdgeThresholdOverride() const
    {
        return m_fxaaQualityEdgeThreshold >= 0.0f;
    }

    bool PostProcessingSettings::HasFXAAQualityEdgeThresholdMinOverride() const
    {
        return m_fxaaQualityEdgeThresholdMin >= 0.0f;
    }

    void PostProcessingSettings::OnGui()
    {
        // Bloom
        ImGui::SeparatorText("Bloom");
        bool overrideBloomStrength = HasBloomStrengthOverride();
        if (ImGui::Checkbox("Override Bloom Strength", &overrideBloomStrength))
        {
            if (overrideBloomStrength && m_bloomStrength < 0.0f)
                m_bloomStrength = 0.05f;  // 기본값
            else if (!overrideBloomStrength)
                m_bloomStrength = -1.0f;
        }
        if (overrideBloomStrength)
        {
            ImGui::DragFloat("Bloom Strength", &m_bloomStrength, 0.01f, 0.0f, 10.0f);
        }

        bool overrideBloomThreshold = HasBloomThresholdOverride();
        if (ImGui::Checkbox("Override Bloom Threshold", &overrideBloomThreshold))
        {
            if (overrideBloomThreshold && m_bloomThreshold < 0.0f)
                m_bloomThreshold = 1.0f;  // 기본값
            else if (!overrideBloomThreshold)
                m_bloomThreshold = -1.0f;
        }
        if (overrideBloomThreshold)
        {
            ImGui::DragFloat("Bloom Threshold", &m_bloomThreshold, 0.1f, 0.0f, 10.0f);
        }

        bool overrideBloomSoftKnee = HasBloomSoftKneeOverride();
        if (ImGui::Checkbox("Override Bloom Soft Knee", &overrideBloomSoftKnee))
        {
            if (overrideBloomSoftKnee && m_bloomSoftKnee < 0.0f)
                m_bloomSoftKnee = 2.0f;  // 기본값
            else if (!overrideBloomSoftKnee)
                m_bloomSoftKnee = -1.0f;
        }
        if (overrideBloomSoftKnee)
        {
            ImGui::DragFloat("Bloom Soft Knee", &m_bloomSoftKnee, 0.1f, 0.0f, 10.0f);
        }

        int enableBloom = m_enableBloom + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
        if (ImGui::Combo("Enable Bloom", &enableBloom, "Use ProjectSettings\0False\0True\0", 3))
        {
            m_enableBloom = enableBloom - 1;  // 0 -> -1, 1 -> 0, 2 -> 1
        }

        // Tone Mapping
        ImGui::SeparatorText("Tone Mapping");
        bool overrideExposure = HasExposureOverride();
        if (ImGui::Checkbox("Override Exposure", &overrideExposure))
        {
            if (overrideExposure && m_exposure < -50.0f)
                m_exposure = -2.5f;  // 기본값
            else if (!overrideExposure)
                m_exposure = -1.0f;
        }
        if (overrideExposure)
        {
            ImGui::DragFloat("Exposure", &m_exposure, 0.1f, -10.0f, 10.0f);
        }

        int enableToneMapping = m_enableToneMapping + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
        if (ImGui::Combo("Enable Tone Mapping", &enableToneMapping, "Use ProjectSettings\0False\0True\0", 3))
        {
            m_enableToneMapping = enableToneMapping - 1;  // 0 -> -1, 1 -> 0, 2 -> 1
        }

        // FXAA
        ImGui::SeparatorText("FXAA");
        bool overrideFXAASubpix = HasFXAAQualitySubpixOverride();
        if (ImGui::Checkbox("Override FXAA Quality Subpix", &overrideFXAASubpix))
        {
            if (overrideFXAASubpix && m_fxaaQualitySubpix < 0.0f)
                m_fxaaQualitySubpix = 0.75f;  // 기본값
            else if (!overrideFXAASubpix)
                m_fxaaQualitySubpix = -1.0f;
        }
        if (overrideFXAASubpix)
        {
            ImGui::DragFloat("FXAA Quality Subpix", &m_fxaaQualitySubpix, 0.01f, 0.0f, 1.0f);
        }

        bool overrideFXAAEdgeThreshold = HasFXAAQualityEdgeThresholdOverride();
        if (ImGui::Checkbox("Override FXAA Quality Edge Threshold", &overrideFXAAEdgeThreshold))
        {
            if (overrideFXAAEdgeThreshold && m_fxaaQualityEdgeThreshold < 0.0f)
                m_fxaaQualityEdgeThreshold = 0.166f;  // 기본값
            else if (!overrideFXAAEdgeThreshold)
                m_fxaaQualityEdgeThreshold = -1.0f;
        }
        if (overrideFXAAEdgeThreshold)
        {
            ImGui::DragFloat("FXAA Quality Edge Threshold", &m_fxaaQualityEdgeThreshold, 0.001f, 0.0f, 1.0f);
        }

        bool overrideFXAAEdgeThresholdMin = HasFXAAQualityEdgeThresholdMinOverride();
        if (ImGui::Checkbox("Override FXAA Quality Edge Threshold Min", &overrideFXAAEdgeThresholdMin))
        {
            if (overrideFXAAEdgeThresholdMin && m_fxaaQualityEdgeThresholdMin < 0.0f)
                m_fxaaQualityEdgeThresholdMin = 0.0833f;  // 기본값
            else if (!overrideFXAAEdgeThresholdMin)
                m_fxaaQualityEdgeThresholdMin = -1.0f;
        }
        if (overrideFXAAEdgeThresholdMin)
        {
            ImGui::DragFloat("FXAA Quality Edge Threshold Min", &m_fxaaQualityEdgeThresholdMin, 0.001f, 0.0f, 1.0f);
        }

        int enableFXAA = m_enableFXAA + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
        if (ImGui::Combo("Enable FXAA", &enableFXAA, "Use ProjectSettings\0False\0True\0", 3))
        {
            m_enableFXAA = enableFXAA - 1;  // 0 -> -1, 1 -> 0, 2 -> 1
        }
    }

    void PostProcessingSettings::Save(json& j) const
    {
        Object::Save(j);

        j["BloomStrength"] = m_bloomStrength;
        j["BloomThreshold"] = m_bloomThreshold;
        j["BloomSoftKnee"] = m_bloomSoftKnee;
        j["EnableBloom"] = m_enableBloom;
        j["Exposure"] = m_exposure;
        j["EnableToneMapping"] = m_enableToneMapping;
        j["FXAAQualitySubpix"] = m_fxaaQualitySubpix;
        j["FXAAQualityEdgeThreshold"] = m_fxaaQualityEdgeThreshold;
        j["FXAAQualityEdgeThresholdMin"] = m_fxaaQualityEdgeThresholdMin;
        j["EnableFXAA"] = m_enableFXAA;
    }

    void PostProcessingSettings::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j, "BloomStrength", m_bloomStrength);
        JsonGet(j, "BloomThreshold", m_bloomThreshold);
        JsonGet(j, "BloomSoftKnee", m_bloomSoftKnee);
        JsonGet(j, "EnableBloom", m_enableBloom);
        JsonGet(j, "Exposure", m_exposure);
        JsonGet(j, "EnableToneMapping", m_enableToneMapping);
        JsonGet(j, "FXAAQualitySubpix", m_fxaaQualitySubpix);
        JsonGet(j, "FXAAQualityEdgeThreshold", m_fxaaQualityEdgeThreshold);
        JsonGet(j, "FXAAQualityEdgeThresholdMin", m_fxaaQualityEdgeThresholdMin);
        JsonGet(j, "EnableFXAA", m_enableFXAA);
    }
}
