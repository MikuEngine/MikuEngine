#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class EnvironmentSettings :
        public Component
    {
        REGISTER_COMPONENT(EnvironmentSettings, Component)

    private:
        // 텍스처 경로 (비어있으면 ProjectSettings 사용)
        std::string m_skyboxTexturePath;
        std::string m_iblIrradiancePath;
        std::string m_iblSpecularPath;
        std::string m_iblBrdfLutPath;

        // 색상 설정 (기본값 -1이면 ProjectSettings 사용)
        Vector3 m_skyboxColor = { -1.0f, -1.0f, -1.0f };
        Vector3 m_skyboxHorizonColor = { -1.0f, -1.0f, -1.0f };
        Vector3 m_iblAmbientColor = { -1.0f, -1.0f, -1.0f };

        // 플래그 (기본값 -1이면 ProjectSettings 사용)
        int m_useSkyboxTexture = -1;  // -1 = use ProjectSettings, 0 = false, 1 = true
        int m_useIBLTexture = -1;
        int m_useIBL = -1;
        float m_iblIntensity = 1.0f;

    public:
        ~EnvironmentSettings();

        void Initialize() override;

    public:
        // 텍스처 경로
        void SetSkyboxTexturePath(const std::string& path);
        void SetIBLIrradiancePath(const std::string& path);
        void SetIBLSpecularPath(const std::string& path);
        void SetIBLBrdfLutPath(const std::string& path);

        const std::string& GetSkyboxTexturePath() const;
        const std::string& GetIBLIrradiancePath() const;
        const std::string& GetIBLSpecularPath() const;
        const std::string& GetIBLBrdfLutPath() const;

        // 색상
        void SetSkyboxColor(const Vector3& color);
        void SetSkyboxHorizonColor(const Vector3& color);
        void SetIBLAmbientColor(const Vector3& color);

        const Vector3& GetSkyboxColor() const;
        const Vector3& GetSkyboxHorizonColor() const;
        const Vector3& GetIBLAmbientColor() const;

        void SetIBLIntensity(float intensity);

        float GetIBLIntensity() const;

        // 플래그
        void SetUseSkyboxTexture(int use);  // -1 = use ProjectSettings
        void SetUseIBLTexture(int use);
        void SetUseIBL(int use);

        int GetUseSkyboxTexture() const;
        int GetUseIBLTexture() const;
        int GetUseIBL() const;

        // 오버라이드 확인
        bool HasSkyboxColorOverride() const;
        bool HasSkyboxHorizonColorOverride() const;
        bool HasIBLAmbientColorOverride() const;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
    };
}
