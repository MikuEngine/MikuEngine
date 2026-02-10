#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Core/System/ProjectSettings.h"

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
    class GeometryShader;
    class ParticleEffect;
    class ParticleEmitter;

    // 투명 렌더링 아이템 (Renderer와 ParticleEmitter 통합)
    struct TransparentRenderItem
    {
        enum class Type { Renderer, ParticleEmitter };
        Type type;
        float distanceSq;

        union
        {
            Renderer* renderer;
            struct
            {
                ParticleEffect* effect;
                const ParticleEmitter* emitter;
            } particle;
        };

        void Render() const;
    };

    class RenderSystem :
        public System<Renderer>
    {
    private:
        std::vector<Renderer*> m_opaqueList;
        std::vector<Renderer*> m_cutoutList;
        std::vector<Renderer*> m_transparentList;
        std::vector<Renderer*> m_screenList;
        std::vector<Renderer*> m_screenDrawList;

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
        std::shared_ptr<GeometryShader> m_shadowPointGS;

        std::shared_ptr<ConstantBuffer> m_localLightCB;
        std::shared_ptr<ConstantBuffer> m_objectCB;
        std::shared_ptr<ConstantBuffer> m_pickingIdCB;
        std::shared_ptr<ConstantBuffer> m_shadowPointCB;
        std::shared_ptr<ConstantBuffer> m_shadowSpotCB;

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

        bool m_screenDirty = true;
        std::uint64_t m_screenSerialCounter = 0;

        float m_prevViewportW = -1.0f;
        float m_prevViewportH = -1.0f;
        bool  m_screenLayoutDirty = true;

        // ProjectSettings 캐시
        ProjectSettings m_projectSettings;
        bool m_projectSettingsLoaded = false;
        
        // 현재 텍스처 경로 (변경 감지용)
        std::string m_currentSkyboxPath;
        std::string m_currentIBLIrradiancePath;
        std::string m_currentIBLSpecularPath;
        std::string m_currentIBLBrdfLutPath;

    public:
        void Register(Renderer* renderer) override;
        void Unregister(Renderer* renderer) override;

    public:
        void Initialize();
        void Update();
        void Render();

        void GetBloomSettings(float& bloomStrength, float& bloomThreshold, float& bloomSoftKnee);
        void SetBloomSettings(float bloomStrength, float bloomThreshold, float bloomSoftKnee);

        const std::vector<Renderer*>& GetRegisteredRenderers() const { return m_components; }

        GameObject* PickObject(int mouseX, int mouseY);

        void MarkScreenDirty();

    private:
        void AddRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type);
        void RemoveRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type);

        void DrawGlobalLightShadow();
        void DrawPointLightShadow();
        void DrawSpotLightShadow();

        void DrawGlobalLight();
        void DrawLocalLight();
        void DrawSkybox();
        void DrawTransparents(const Vector3& cameraPosition);

        std::vector<Renderer*> RebuildScreenDrawList(const std::vector<Renderer*>& v);
        void UpdateCanvasLayout();

        // 헬퍼 메서드: 텍스처 경로
        std::string GetSkyboxTexturePath() const;
        std::string GetIBLIrradiancePath() const;
        std::string GetIBLSpecularPath() const;
        std::string GetIBLBrdfLutPath() const;

        // 헬퍼 메서드: 색상
        Vector3 GetSkyboxColor() const;
        Vector3 GetSkyboxHorizonColor() const;
        Vector3 GetIBLAmbientColor() const;

        // 헬퍼 메서드: 플래그
        bool GetUseSkyboxTexture() const;
        bool GetUseIBLTexture() const;
        bool GetUseIBL() const;
        float GetIBLIntensity() const;

        // 헬퍼 메서드: 포스트프로세싱
        float GetBloomStrength() const;
        float GetBloomThreshold() const;
        float GetBloomSoftKnee() const;
        bool GetEnableBloom() const;
        float GetExposure() const;
        bool GetEnableToneMapping() const;
        float GetFXAAQualitySubpix() const;
        float GetFXAAQualityEdgeThreshold() const;
        float GetFXAAQualityEdgeThresholdMin() const;
        bool GetEnableFXAA() const;

        // 텍스처 변경 감지 및 재로딩
        void CheckAndReloadTextures();
    };
}
