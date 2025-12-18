#include "pch.h"
#include "SamplerState.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    void SamplerState::Create(const D3D11_SAMPLER_DESC& desc)
    {
        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateSamplerState(&desc, &m_samplerState));
    }

    const Microsoft::WRL::ComPtr<ID3D11SamplerState>& SamplerState::GetSamplerState() const
    {
        return m_samplerState;
    }

    ID3D11SamplerState* SamplerState::GetRawSamplerState() const
    {
        return m_samplerState.Get();
    }
}