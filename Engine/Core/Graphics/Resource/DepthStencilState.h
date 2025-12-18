#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
    class DepthStencilState :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    public:
        void Create(const D3D11_DEPTH_STENCIL_DESC& desc);

    public:
        const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& GetDepthStencilState() const;
        ID3D11DepthStencilState* GetRawDepthStencilState() const;
    };
}