#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
    class BlendState :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;

    public:
        void Create(const D3D11_BLEND_DESC& desc);

    public:
        const Microsoft::WRL::ComPtr<ID3D11BlendState>& GetBlendState() const;
        ID3D11BlendState* GetRawBlendState() const;
    };
}