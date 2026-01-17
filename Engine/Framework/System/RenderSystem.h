#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Renderer.h"

namespace engine
{
    class ConstantBuffer;
    class SamplerState;
    class Texture;
    class RasterizerState;
    class DepthStencilState;
    class StaticMeshData;
    class VertexBuffer;
    class VertexShader;
    class IndexBuffer;
    class InputLayout;
    class PixelShader;
    class BlendState;
    class GameObject;

    class RenderSystem :
        public System<Renderer>
    {
    private:
        std::vector<Renderer*> m_opaqueList;
        std::vector<Renderer*> m_cutoutList;
        std::vector<Renderer*> m_transparentList;
        std::vector<Renderer*> m_screenList;

        std::shared_ptr<ConstantBuffer> m_frameCB;
        std::shared_ptr<SamplerState> m_comparisonSamplerState;
        std::shared_ptr<SamplerState> m_clampSamplerState;
        std::shared_ptr<SamplerState> m_linearSamplerState;

        std::shared_ptr<Texture> m_skyboxEnv;
        std::shared_ptr<Texture> m_irradianceMap;
        std::shared_ptr<Texture> m_specularMap;
        std::shared_ptr<Texture> m_brdfLut;

        //skybox
        std::shared_ptr<VertexBuffer> m_cubeVertexBuffer;
        std::shared_ptr<IndexBuffer> m_cubeIndexBuffer;
        std::shared_ptr<InputLayout> m_cubeInputLayout;
        std::shared_ptr<VertexShader> m_skyboxVertexShader;
        std::shared_ptr<PixelShader> m_skyboxPixelShader;
        std::shared_ptr<RasterizerState> m_skyboxRSState;
        std::shared_ptr<DepthStencilState> m_skyboxDSState;

        // light
        std::shared_ptr<VertexBuffer> m_sphereVB;
        std::shared_ptr<IndexBuffer> m_sphereIB;
        std::shared_ptr<VertexBuffer> m_coneVB;
        std::shared_ptr<IndexBuffer> m_coneIB;

        std::shared_ptr<VertexShader> m_lightVolumeVS;
        std::shared_ptr<PixelShader> m_pointLightPS;
        std::shared_ptr<PixelShader> m_spotLightPS;
        std::shared_ptr<InputLayout> m_lightVolumeInputLayout;
        std::shared_ptr<ConstantBuffer> m_localLightCB;
        std::shared_ptr<ConstantBuffer> m_objectCB;
        std::shared_ptr<ConstantBuffer> m_pickingIdCB;

        std::shared_ptr<BlendState> m_additiveBS;
        std::shared_ptr<RasterizerState> m_frontRSS;
        std::shared_ptr<DepthStencilState> m_lightVolumeDSS;

        // transparent
        std::shared_ptr<BlendState> m_transparentBlendState;
        std::shared_ptr<DepthStencilState> m_transparentDSState;

        float m_bloomStrength = 0.05f;
        float m_bloomThreshold = 1.0f;
        float m_bloomSoftKnee = 2.0f;
        float m_exposure = -2.0f;

    public:
        RenderSystem();

    public:
        void Register(Renderer* renderer) override;
        void Unregister(Renderer* renderer) override;

    public:
        void Update();
        void Render();

        void GetBloomSettings(float& bloomStrength, float& bloomThreshold, float& bloomSoftKnee);
        void SetBloomSettings(float bloomStrength, float bloomThreshold, float bloomSoftKnee);

        GameObject* PickObject(int mouseX, int mouseY);

    private:
        void AddRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type);
        void RemoveRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type);

        void DrawGlobalLight();
        void DrawLocalLight();
        void DrawSkybox();
        void DrawTransparents(const Vector3& cameraPosition);
    };
}
