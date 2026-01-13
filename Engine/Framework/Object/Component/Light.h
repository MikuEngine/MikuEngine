#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    enum class LightType
    {
        Directional,
        Point,
        Spot
    };

    class Light :
        public Component
    {
        REGISTER_COMPONENT(Light)

    private:
        LightType m_lightType = LightType::Directional;
        Vector3 m_color{ 1.0f, 1.0f, 1.0f };
        float m_intensity = 1.0f;
        float m_lightNear = 90000.0f;
        float m_lightFar = 100000.0f;
        float m_lightForwardDist = 1000.0f;
        float m_lightHeightRatio = 0.9f;
        float m_range = 10.0f;
        float m_angle = 45.0f;

    public:
        ~Light();

        void Initialize() override;

    public:
        void SetLightType(LightType lightType);
        void SetColor(const Vector3& color);
        void SetIntensity(float intensity);
        void SetRange(float range);
        void SetAngle(float angle);
        void SetLightNear(float lightNear);
        void SetLightFar(float lightFar);
        void SetForwardDist(float forwardDist);
        void SetHeightRatio(float heightRatio);

        LightType GetLightType() const;
        const Vector3& GetColor() const;
        float GetIntensity() const;
        float GetRange() const;
        float GetAngle() const;
        float GetLightNear() const;
        float GetLightFar() const;
        float GetForwardDist() const;
        float GetHeightRatio() const;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;
    };
}