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
        std::string m_meshFilePath;
        std::string m_shaderFilePath;

        std::shared_ptr<StaticMeshData> m_staticMeshData;
        std::shared_ptr<MaterialData> m_materialData;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;
        std::shared_ptr<ConstantBuffer> m_materialConstantBuffer;
        std::shared_ptr<ConstantBuffer> m_objectConstantBuffer;
        std::shared_ptr<VertexShader> m_finalPassVertexShader;
        std::shared_ptr<VertexShader> m_shadowPassVertexShader;
        std::shared_ptr<PixelShader> m_finalPassPixelShader;
        std::shared_ptr<PixelShader> m_shadowPassPixelShader;
        std::vector<Textures> m_textures;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;

    public:
        StaticMeshRenderer() = default;
        StaticMeshRenderer(const std::string& meshFilePath, const std::string& shaderFilePath);
        ~StaticMeshRenderer();

    public:
        void Initialize() override;

        void SetMesh(const std::string& meshFilePath);
        void SetPixelShader(const std::string& shaderFilePath);

    public:
        void OnGui() override {}
        void Save(json& j) const override {}
        void Load(const json& j) override {}
        std::string GetType() const override;

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw() const override;
        DirectX::BoundingBox GetBounds() const override;
        //void DrawShadow() const;
    };
}