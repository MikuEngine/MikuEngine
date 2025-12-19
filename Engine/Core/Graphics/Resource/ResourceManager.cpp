#include "pch.h"
#include "ResourceManager.h"

#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Resource/BlendState.h"

namespace engine
{
    ResourceManager::~ResourceManager() = default;

    void ResourceManager::Initialize()
    {
        CreateDefaultTextures();
        CreateDefaultSamplerStates();
        CreateDefaultRasterizerStates();
        CreateDefaultDepthStencilStates();
        CreateDefaultBlendStates();
    }

    void ResourceManager::Cleanup()
    {
        // 직접 use_count 감소
        m_defaultTextures.fill(nullptr);
        m_defaultSamplerStates.fill(nullptr);
        m_defaultRasterizerStates.fill(nullptr);
        m_defaultDepthStencilStates.fill(nullptr);
        m_defaultBlendStates.fill(nullptr);
    }

    std::shared_ptr<IndexBuffer> ResourceManager::GetOrCreateIndexBuffer(const std::string& filePath, const std::vector<DWORD>& indices)
    {
        if (auto find = m_indexBuffers.find(filePath); find != m_indexBuffers.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto indexBuffer = std::make_shared<IndexBuffer>();
        indexBuffer->Create(indices);

        m_indexBuffers[filePath] = indexBuffer;

        return indexBuffer;
    }

    std::shared_ptr<Texture> ResourceManager::GetOrCreateTexture(const std::string& filePath)
    {
        if (auto find = m_textures.find(filePath); find != m_textures.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto texture = std::make_shared<Texture>();
        texture->Create(filePath);

        m_textures[filePath] = texture;

        return texture;
    }

    std::shared_ptr<Texture> ResourceManager::GetOrCreateTexture(
        const std::string& name,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT bindFlags)
    {
        if (auto find = m_textures.find(name); find != m_textures.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto texture = std::make_shared<Texture>();
        texture->Create(width, height, format, bindFlags);

        m_textures[name] = texture;

        return texture;
    }

    std::shared_ptr<Texture> ResourceManager::GetOrCreateTexture(
        const std::string& name,
        const D3D11_TEXTURE2D_DESC& desc,
        DXGI_FORMAT srvFormat,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat)
    {
        if (auto find = m_textures.find(name); find != m_textures.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto texture = std::make_shared<Texture>();
        texture->Create(desc, srvFormat, rtvFormat, dsvFormat);

        m_textures[name] = texture;

        return texture;
    }

    std::shared_ptr<Texture> ResourceManager::GetDefaultTexture(DefaultTextureType type)
    {
        return m_defaultTextures[static_cast<size_t>(type)];
    }

    std::shared_ptr<SamplerState> ResourceManager::GetDefaultSamplerState(DefaultSamplerType type)
    {
        return m_defaultTextures[static_cast<size_t>(type)];
    }

    std::shared_ptr<RasterizerState> ResourceManager::GetDefaultRasterizerState(DefaultRasterizerType type)
    {
        return m_defaultTextures[static_cast<size_t>(type)];
    }

    std::shared_ptr<DepthStencilState> ResourceManager::GetDefaultDepthStencilState(DefaultDepthStencilType type)
    {
        return m_defaultTextures[static_cast<size_t>(type)];
    }

    std::shared_ptr<BlendState> ResourceManager::GetDefaultBlendState(const std::string& filePath)
    {
        return m_defaultTextures[static_cast<size_t>(type)];
    }

    std::shared_ptr<ConstantBuffer> ResourceManager::GetOrCreateConstantBuffer(const std::string& name, UINT byteWidth)
    {
        if (auto find = m_constantBuffers.find(name); find != m_constantBuffers.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto constantBuffer = std::make_shared<ConstantBuffer>();
        constantBuffer->Create(byteWidth);

        m_constantBuffers[name] = constantBuffer;

        return constantBuffer;
    }

    std::shared_ptr<VertexShader> ResourceManager::GetOrCreateVertexShader(const std::string& filePath)
    {
        if (auto find = m_vertexShaders.find(filePath); find != m_vertexShaders.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto vertexShader = std::make_shared<VertexShader>();
        vertexShader->Create(filePath);

        m_vertexShaders[filePath] = vertexShader;

        return vertexShader;
    }

    std::shared_ptr<PixelShader> ResourceManager::GetOrCreatePixelShader(const std::string& filePath)
    {
        if (auto find = m_pixelShaders.find(filePath); find != m_pixelShaders.end())
        {
            if (!find->second.expired())
            {
                return find->second.lock();
            }
        }

        auto pixelShader = std::make_shared<PixelShader>();
        pixelShader->Create(filePath);

        m_pixelShaders[filePath] = pixelShader;

        return pixelShader;
    }

    std::shared_ptr<SamplerState> ResourceManager::GetOrCreateSamplerState(const std::string& filePath)
    {
        return std::shared_ptr<SamplerState>();
    }

    std::shared_ptr<RasterizerState> ResourceManager::GetOrCreateRasterizerState(const std::string& filePath)
    {
        return std::shared_ptr<RasterizerState>();
    }

    std::shared_ptr<DepthStencilState> ResourceManager::GetOrCreateDepthStencilState(const std::string& filePath)
    {
        return std::shared_ptr<DepthStencilState>();
    }

    std::shared_ptr<BlendState> ResourceManager::GetOrCreateBlendState(const std::string& filePath)
    {
        return std::shared_ptr<BlendState>();
    }

    void ResourceManager::CreateDefaultTextures()
    {
        // 흰색
        m_defaultTextures[static_cast<size_t>(DefaultTextureType::White)] = std::make_shared<Texture>();
        m_defaultTextures[static_cast<size_t>(DefaultTextureType::White)]->Create(std::array<unsigned char, 4>{ 255, 255, 255, 255 });

        // 검은색
        auto black = std::make_shared<Texture>();
        black->Create(std::array<unsigned char, 4>{ 0, 0, 0, 255 });
        m_defaultTextures[static_cast<size_t>(DefaultTextureType::Black)] = std::make_shared<Texture>();
        m_defaultTextures[static_cast<size_t>(DefaultTextureType::Black)]->Create(std::array<unsigned char, 4>{ 0, 0, 0, 255 });

        // 기본 노말
        m_defaultTextures[static_cast<size_t>(DefaultTextureType::Normal)] = std::make_shared<Texture>();
        m_defaultTextures[static_cast<size_t>(DefaultTextureType::Normal)]->Create(std::array<unsigned char, 4>{ 128, 128, 255, 255 });
    }

    void ResourceManager::CreateDefaultSamplerStates()
    {
        // Point (Linear보다 빠름 / 픽셀 아트)
        {
            D3D11_SAMPLER_DESC desc{};
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            m_defaultSamplerStates[static_cast<size_t>(DefaultSamplerType::Point)] = std::make_shared<SamplerState>();
            m_defaultSamplerStates[static_cast<size_t>(DefaultSamplerType::Point)]->Create(desc);
        }
        // Linear (기본값)
        {
            D3D11_SAMPLER_DESC desc{};
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            m_defaultSamplerStates[static_cast<size_t>(DefaultSamplerType::Linear)] = std::make_shared<SamplerState>();
            m_defaultSamplerStates[static_cast<size_t>(DefaultSamplerType::Linear)]->Create(desc);
        }
        // Anisotropic (고품질)
        {
            D3D11_SAMPLER_DESC desc{};
            desc.Filter = D3D11_FILTER_ANISOTROPIC;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.MaxAnisotropy = 16;
            m_defaultSamplerStates[static_cast<size_t>(DefaultSamplerType::Anisotropic)] = std::make_shared<SamplerState>();
            m_defaultSamplerStates[static_cast<size_t>(DefaultSamplerType::Anisotropic)]->Create(desc);
        }
    }

    void ResourceManager::CreateDefaultRasterizerStates()
    {
        // SolidBack (기본값)
        {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_BACK;
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::SolidBack)] = std::make_shared<RasterizerState>();
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::SolidBack)]->Create(desc);
        }
        // SolidFront (반전면, skybox)
        {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_FRONT;
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::SolidFront)] = std::make_shared<RasterizerState>();
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::SolidFront)]->Create(desc);
        }
        // SolidNone (양면)
        {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::SolidNone)] = std::make_shared<RasterizerState>();
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::SolidNone)]->Create(desc);
        }
        // Wireframe (디버그용)
        {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = D3D11_FILL_WIREFRAME;
            desc.CullMode = D3D11_CULL_NONE;
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::Wireframe)] = std::make_shared<RasterizerState>();
            m_defaultRasterizerStates[static_cast<size_t>(DefaultRasterizerType::Wireframe)]->Create(desc);
        }
    }

    void ResourceManager::CreateDefaultDepthStencilStates()
    {
        // Disabled (불투명)
        {
            D3D11_BLEND_DESC desc{}; // 기본값 = FALSE (Disabled)
            desc.RenderTarget[0].BlendEnable = FALSE;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            m_defaultBlendStates[static_cast<size_t>(DefaultBlendType::Disabled)] = std::make_shared<BlendState>();
            m_defaultBlendStates[static_cast<size_t>(DefaultBlendType::Disabled)]->Create(desc);
        }
        // AlphaBlend (투명)
        {
            D3D11_BLEND_DESC desc{};
            desc.RenderTarget[0].BlendEnable = TRUE;
            desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            m_defaultBlendStates[static_cast<size_t>(DefaultBlendType::AlphaBlend)] = std::make_shared<BlendState>();
            m_defaultBlendStates[static_cast<size_t>(DefaultBlendType::AlphaBlend)]->Create(desc);
        }
        // Additive (더하기 - 이펙트)
        {
            D3D11_BLEND_DESC desc{};
            desc.RenderTarget[0].BlendEnable = TRUE;
            desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // or ONE
            desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            // ... Alpha settings
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            m_defaultBlendStates[static_cast<size_t>(DefaultBlendType::Additive)] = std::make_shared<BlendState>();
            m_defaultBlendStates[static_cast<size_t>(DefaultBlendType::Additive)]->Create(desc);
        }
    }

    void ResourceManager::CreateDefaultBlendStates()
    {
        // Less (기본)
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_LESS;
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::Less)] = std::make_shared<DepthStencilState>();
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::Less)]->Create(desc);
        }
        // LessEqual (스카이박스 등)
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::LessEqual)] = std::make_shared<DepthStencilState>();
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::LessEqual)]->Create(desc);
        }
        // DepthRead (깊이 읽기만 함)
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 쓰기 끔
            desc.DepthFunc = D3D11_COMPARISON_LESS; // 또는 LESS_EQUAL
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::DepthRead)] = std::make_shared<DepthStencilState>();
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::DepthRead)]->Create(desc);
        }
        // None (깊이 끔 - UI/2D)
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = FALSE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc = D3D11_COMPARISON_LESS; // 의미 없음
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::None)] = std::make_shared<DepthStencilState>();
            m_defaultDepthStencilStates[static_cast<size_t>(DefaultDepthStencilType::None)]->Create(desc);
        }
    }
}