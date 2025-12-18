#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
    class SamplerState :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

    public:
        void Create(const D3D11_SAMPLER_DESC& desc);

    public:
        const Microsoft::WRL::ComPtr<ID3D11SamplerState>& GetSamplerState() const;
        ID3D11SamplerState* GetRawSamplerState() const;
    };
}