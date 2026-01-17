#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
    class Texture :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
        D3D11_TEXTURE2D_DESC m_desc{};

    public:
        void Create(const std::string& filePath);
        void Create(UINT width, UINT height, DXGI_FORMAT format, UINT bindFlags);
        void Create(
            const D3D11_TEXTURE2D_DESC& desc,
            DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT rtvFormat = DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
        void Create(const std::array<unsigned char, 4>& color);

    public:
        const Microsoft::WRL::ComPtr<ID3D11Texture2D>& GetTexture() const;
        const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetSRV() const;
        const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& GetRTV() const;
        const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& GetDSV() const;

        ID3D11Texture2D* GetRawTexture() const;
        ID3D11ShaderResourceView* GetRawSRV() const;
        ID3D11RenderTargetView* GetRawRTV() const;
        ID3D11DepthStencilView* GetRawDSV() const;

        float GetWidth() const;
        float GetHeight() const;
    };

    struct Textures
    {
        std::shared_ptr<Texture> baseColor;
        std::shared_ptr<Texture> normal;
        std::shared_ptr<Texture> metalness;
        std::shared_ptr<Texture> roughness;
        std::shared_ptr<Texture> ambientOcclusion;
        std::shared_ptr<Texture> emissive;

        std::array<ID3D11ShaderResourceView*, 6> AsRawArray() const
        {
            return {
                baseColor->GetRawSRV(),
                normal->GetRawSRV(),
                metalness->GetRawSRV(),
                roughness->GetRawSRV(),
                ambientOcclusion->GetRawSRV(),
                emissive->GetRawSRV(),
            };
        }
    };
}