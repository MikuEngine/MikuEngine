#pragma once

#include "Framework/Object/Component/Renderer.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Common/Type/DefaultAssetTypes.h"

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
    private:
        std::string m_meshFilePath;
        std::string m_shaderFilePath;

        std::shared_ptr<StaticMeshData> m_staticMeshData;
        std::shared_ptr<MaterialData> m_materialData;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;
        std::shared_ptr<ConstantBuffer> m_materialBuffer;
        std::shared_ptr<ConstantBuffer> m_worldTransformBuffer;
        std::shared_ptr<VertexShader> m_finalPassVertexShader;
        std::shared_ptr<VertexShader> m_shadowPassVertexShader;
        std::shared_ptr<PixelShader> m_finalPassPixelShader;
        std::shared_ptr<PixelShader> m_shadowPassPixelShader;
        std::vector<Textures> m_textures;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;
        std::shared_ptr<SamplerState> m_comparisonSamplerState;

    public:
        StaticMeshRenderer();
        StaticMeshRenderer(const std::string& meshFilePath, const std::string& shaderFilePath);
        StaticMeshRenderer(DefaultStaticMeshType type, const std::string& shaderFilePath);
        ~StaticMeshRenderer();

    public:
        void SetMesh(const std::string& meshFilePath);
        void SetPixelShader(const std::string& shaderFilePath);

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw() const override;
        DirectX::BoundingBox GetBounds() const override;
        //void DrawShadow() const;
    };
}