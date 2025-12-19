#pragma once

#include "Core/Graphics/Resource/ResourceKey.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/DefaultResourceTypes.h"

namespace engine
{
    class GraphicsDevice;
    class VertexBuffer;
    class IndexBuffer;
    class Texture;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class SamplerState;
    class RasterizerState;
    class DepthStencilState;
    class BlendState;

    template <typename T>
    concept IsVertex = requires { T::vertexFormat; };

    class ResourceManager :
        public Singleton<ResourceManager>
    {
    private:
        std::unordered_map<VertexBufferKey, std::weak_ptr<VertexBuffer>> m_vertexBuffers;
        std::unordered_map<std::string, std::weak_ptr<IndexBuffer>> m_indexBuffers;
        std::unordered_map<std::string, std::weak_ptr<Texture>> m_textures;
        std::unordered_map<std::string, std::weak_ptr<ConstantBuffer>> m_constantBuffers;
        std::unordered_map<std::string, std::weak_ptr<VertexShader>> m_vertexShaders;
        std::unordered_map<std::string, std::weak_ptr<PixelShader>> m_pixelShaders;
        std::unordered_map<std::string, std::weak_ptr<SamplerState>> m_samplerStates;
        std::unordered_map<std::string, std::weak_ptr<RasterizerState>> m_rasterizerStates;
        std::unordered_map<std::string, std::weak_ptr<DepthStencilState>> m_depthStencilStates;
        std::unordered_map<std::string, std::weak_ptr<BlendState>> m_blendStates;

        // default resources
        std::array<std::shared_ptr<Texture>, static_cast<size_t>(DefaultTextureType::Count)> m_defaultTextures;
        std::array<std::shared_ptr<SamplerState>, static_cast<size_t>(DefaultSamplerType::Count)> m_defaultSamplerStates;
        std::array<std::shared_ptr<RasterizerState>, static_cast<size_t>(DefaultRasterizerType::Count)> m_defaultRasterizerStates;
        std::array<std::shared_ptr<DepthStencilState>, static_cast<size_t>(DefaultDepthStencilType::Count)> m_defaultDepthStencilStates;
        std::array<std::shared_ptr<BlendState>, static_cast<size_t>(DefaultBlendType::Count)> m_defaultBlendStates;

    private:
        ResourceManager() = default;
        ~ResourceManager();

    public:
        void Initialize();
        void Cleanup();

    public:
        // VertexBuffer는 버텍스 타입별로 구분이 필요하기 때문에 템플릿으로 만듦
        template <IsVertex T>
        std::shared_ptr<VertexBuffer> GetOrCreateVertexBuffer(const std::string& filePath, const std::vector<T>& vertices)
        {
            VertexBufferKey key{ filePath, T::vertexFormat };
            if (auto find = m_vertexBuffers.find(key); find != m_vertexBuffers.end())
            {
                if (!find->second.expired())
                {
                    return find->second.lock();
                }
            }

            std::shared_ptr<VertexBuffer> vertexBuffer = std::make_shared<VertexBuffer>();
            vertexBuffer->Create(vertices);

            m_vertexBuffers[key] = vertexBuffer;

            return vertexBuffer;
        }

        std::shared_ptr<IndexBuffer> GetOrCreateIndexBuffer(const std::string& filePath, const std::vector<DWORD>& indices);
        std::shared_ptr<Texture> GetOrCreateTexture(const std::string& filePath);
        std::shared_ptr<Texture> GetOrCreateTexture(
            const std::string& name,
            UINT width,
            UINT height,
            DXGI_FORMAT format,
            UINT bindFlags);

        std::shared_ptr<Texture> GetOrCreateTexture(
            const std::string& name,
            const D3D11_TEXTURE2D_DESC& desc,
            DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT rtvFormat = DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);

        std::shared_ptr<ConstantBuffer> GetOrCreateConstantBuffer(const std::string& name, UINT byteWidth);
        std::shared_ptr<VertexShader> GetOrCreateVertexShader(const std::string& filePath);
        std::shared_ptr<PixelShader> GetOrCreatePixelShader(const std::string& filePath);

        std::shared_ptr<SamplerState> GetOrCreateSamplerState(const std::string& name, const D3D11_SAMPLER_DESC& desc);
        std::shared_ptr<RasterizerState> GetOrCreateRasterizerState(const std::string& name, const D3D11_RASTERIZER_DESC& desc);
        std::shared_ptr<DepthStencilState> GetOrCreateDepthStencilState(const std::string& name, const D3D11_DEPTH_STENCIL_DESC& desc);
        std::shared_ptr<BlendState> GetOrCreateBlendState(const std::string& name, const D3D11_BLEND_DESC& desc);

        // default
        std::shared_ptr<Texture> GetDefaultTexture(DefaultTextureType type);
        std::shared_ptr<SamplerState> GetDefaultSamplerState(DefaultSamplerType type);
        std::shared_ptr<RasterizerState> GetDefaultRasterizerState(DefaultRasterizerType type);
        std::shared_ptr<DepthStencilState> GetDefaultDepthStencilState(DefaultDepthStencilType type);
        std::shared_ptr<BlendState> GetDefaultBlendState(DefaultBlendType type);

    private:
        void CreateDefaultTextures();
        void CreateDefaultSamplerStates();
        void CreateDefaultRasterizerStates();
        void CreateDefaultDepthStencilStates();
        void CreateDefaultBlendStates();

    private:
        friend class Singleton<ResourceManager>;
    };
}