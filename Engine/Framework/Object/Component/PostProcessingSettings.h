#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class PostProcessingSettings :
        public Component
    {
        REGISTER_COMPONENT(PostProcessingSettings, Component)

    private:
        // Bloom
        float m_bloomStrength = -1.0f;  // -1 = use ProjectSettings
        float m_bloomThreshold = -1.0f;
        float m_bloomSoftKnee = -1.0f;
        int m_enableBloom = -1;

        // Tone Mapping
        float m_exposure = -1.0f;
        int m_enableToneMapping = -1;

        // FXAA
        float m_fxaaQualitySubpix = -1.0f;
        float m_fxaaQualityEdgeThreshold = -1.0f;
        float m_fxaaQualityEdgeThresholdMin = -1.0f;
        int m_enableFXAA = -1;

    public:
        ~PostProcessingSettings();

        void Initialize() override;

    public:
        // Bloom
        void SetBloomStrength(float strength);
        void SetBloomThreshold(float threshold);
        void SetBloomSoftKnee(float softKnee);
        void SetEnableBloom(int enable);

        float GetBloomStrength() const;
        float GetBloomThreshold() const;
        float GetBloomSoftKnee() const;
        int GetEnableBloom() const;

        // Tone Mapping
        void SetExposure(float exposure);
        void SetEnableToneMapping(int enable);

        float GetExposure() const;
        int GetEnableToneMapping() const;

        // FXAA
        void SetFXAAQualitySubpix(float value);
        void SetFXAAQualityEdgeThreshold(float value);
        void SetFXAAQualityEdgeThresholdMin(float value);
        void SetEnableFXAA(int enable);

        float GetFXAAQualitySubpix() const;
        float GetFXAAQualityEdgeThreshold() const;
        float GetFXAAQualityEdgeThresholdMin() const;
        int GetEnableFXAA() const;

        // 오버라이드 확인
        bool HasBloomStrengthOverride() const;
        bool HasBloomThresholdOverride() const;
        bool HasBloomSoftKneeOverride() const;
        bool HasExposureOverride() const;
        bool HasFXAAQualitySubpixOverride() const;
        bool HasFXAAQualityEdgeThresholdOverride() const;
        bool HasFXAAQualityEdgeThresholdMinOverride() const;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
    };
}
