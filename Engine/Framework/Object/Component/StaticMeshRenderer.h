#pragma once

#include "Framework/Object/Component/Renderer.h"
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

    class StaticMeshRenderer :
        public Renderer
    {
        REGISTER_COMPONENT(StaticMeshRenderer)

    private:
        std::shared_ptr<StaticMeshData> m_staticMeshData;
        std::shared_ptr<MaterialData> m_materialData;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;

        std::shared_ptr<ConstantBuffer> m_materialConstantBuffer;
        std::shared_ptr<ConstantBuffer> m_objectConstantBuffer;

        std::shared_ptr<VertexShader> m_vs;
        std::shared_ptr<VertexShader> m_shadowVS;
        std::shared_ptr<VertexShader> m_simpleVS;

        std::shared_ptr<PixelShader> m_opaquePS;
        std::shared_ptr<PixelShader> m_cutoutPS;
        std::shared_ptr<PixelShader> m_transparentPS;
        std::shared_ptr<PixelShader> m_maskCutoutPS;

        std::vector<Textures> m_textures;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;

        std::string m_meshFilePath;
        std::string m_vsFilePath;
        std::string m_opaquePSFilePath;
        std::string m_cutoutPSFilePath;
        std::string m_transparentPSFilePath;

        Vector4 m_materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector3 m_materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
        float m_materialRoughness = 0.0f;
        float m_materialMetalness = 0.0f;
        float m_materialAmbientOcclusion = 1.0f;
        bool m_overrideMaterial = false;

    public:
        ~StaticMeshRenderer();

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

    public:
        void Initialize() override;

        void SetMesh(const std::string& meshFilePath);
        void SetVertexShader(const std::string& shaderFilePath);
        void SetOpaquePixelShader(const std::string& shaderFilePath);
        void SetCutoutPixelShader(const std::string& shaderFilePath);
        void SetTransparentPixelShader(const std::string& shaderFilePath);

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        DirectX::BoundingBox GetBounds() const override;
        void DrawMask() const override;

    private:
        void Refresh();
    };
}