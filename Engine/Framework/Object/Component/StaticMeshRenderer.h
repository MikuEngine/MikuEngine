#pragma once

#include "Framework/Object/Component/Component.h"
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
        public Component
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

    public:
        void SetMesh(const std::string& meshFilePath);
        void SetPixelShader(const std::string& shaderFilePath);

    public:
        void Draw() const;
        void DrawShadow() const;
    };
}