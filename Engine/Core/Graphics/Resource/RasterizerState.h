#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
    class RasterizerState :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;

    public:
        void Create(const D3D11_RASTERIZER_DESC& desc);

    public:
        const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& GetRasterizerState() const;
        ID3D11RasterizerState* GetRawRasterizerState() const;
    };
}