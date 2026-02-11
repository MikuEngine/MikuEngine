#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Core/Graphics/Resource/Texture.h"

namespace engine
{
    class StaticMeshData;
    class MaterialData;

    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class Texture;
    class InputLayout;
    class SamplerState;
    class RasterizerState;

    class StaticMeshRenderer :
        public Renderer
    {
        REGISTER_COMPONENT(StaticMeshRenderer, Renderer)

    private:
        std::shared_ptr<StaticMeshData> m_staticMeshData;
        std::shared_ptr<MaterialData> m_materialData;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;

        std::shared_ptr<ConstantBuffer> m_materialConstantBuffer;
        std::shared_ptr<ConstantBuffer> m_objectConstantBuffer;

        std::shared_ptr<VertexShader> m_vs;
        std::shared_ptr<VertexShader> m_transparentVS;  // null이면 투명 패스에 m_vs 사용
        std::shared_ptr<VertexShader> m_shadowVS;
        std::shared_ptr<VertexShader> m_spotShadowVS;
        std::shared_ptr<VertexShader> m_pointShadowVS;
        std::shared_ptr<VertexShader> m_simpleVS;

        std::shared_ptr<PixelShader> m_opaquePS;
        std::shared_ptr<PixelShader> m_cutoutPS;
        std::shared_ptr<PixelShader> m_transparentPS;
        std::shared_ptr<PixelShader> m_maskCutoutPS;
        std::shared_ptr<PixelShader> m_pickingPS;
        std::shared_ptr<PixelShader> m_pointShadowPS;
        std::shared_ptr<PixelShader> m_pointShadowCutoutPS;

        std::vector<Textures> m_textures;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;
        std::shared_ptr<RasterizerState> m_rasterizerState;

        std::string m_meshFilePath;
        std::string m_vsFilePath;
        std::string m_transparentVSFilePath;  // 비어 있으면 투명 패스에 m_vs 사용
        std::string m_opaquePSFilePath;
        std::string m_cutoutPSFilePath;
        std::string m_transparentPSFilePath;

        // SetCustomBuffer로 설정한 데이터 (투명 패스에서 해당 슬롯에 바인딩)
        int m_customBufferSlot = -1;
        std::vector<std::uint8_t> m_customBufferData;
        size_t m_customBufferAlignedSize = 0;
        std::shared_ptr<ConstantBuffer> m_customConstantBuffer;

        Vector4 m_materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector3 m_materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
        float m_materialRoughness = 1.0f;
        float m_materialMetalness = 0.0f;
        float m_materialAmbientOcclusion = 1.0f;
        float m_materialEmissiveIntensity = 1.0f;
        bool m_overrideMaterial = false;
        bool m_castShadow = false;
        bool m_isInitialized = false;
        CullMode m_cullMode = CullMode::Back;

        // 장애물 반투명 처리
        bool m_useObstacleTransparency = false;
        float m_obstacleAlpha = 1.0f;
        float m_subsurfaceStrength = 0.0f;  // SSS strength for whole mesh (0 = off), GBuffer ORM.a. Later: texture modulates.
        Vector3 m_subsurfaceColor = Vector3(1.0f, 1.0f, 1.0f);  // SSS tint per renderer (RGB). (0,0,0) = use global CbFrame subsurfaceColor.

        mutable std::vector<ID3D11RasterizerState*> m_cachedPasses;
        mutable bool m_isPassDirty = true; // 상태 변경 확인용 플래그

    public:
        ~StaticMeshRenderer();

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

    public:
        void Initialize() override;
        void Update();

        void SetMesh(const std::string& meshFilePath);
        void SetVertexShader(const std::string& shaderFilePath);
        void SetTransparentVertexShader(const std::string& shaderFilePath);
        void SetTransparentShader(const std::string& vsFilePath, const std::string& psFilePath);
        void SetOpaquePixelShader(const std::string& shaderFilePath);
        void SetCutoutPixelShader(const std::string& shaderFilePath);
        void SetTransparentPixelShader(const std::string& shaderFilePath);
        void SetCustomBuffer(int slot, const void* data, size_t byteSize);
        void SetCastShadow(bool cast);;
        bool IsCastShadow() const override;
        void SetCullMode(CullMode cullMode);

        // 장애물 반투명 설정 (스크립트에서 호출)
        void SetObstacleAlpha(bool enable, float alpha);

        const std::string& GetMeshPath() const override;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        DirectX::BoundingBox GetBounds() const override;
        void DrawShadow(RenderType renderType, LightType lightType) const override;
        void DrawMask() const override;
        void DrawPickingID() const override;

        void UpdateSockets() override;

    private:
        void Refresh();
    };
}