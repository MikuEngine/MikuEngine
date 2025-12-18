#include "pch.h"
#include "BlendState.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    void BlendState::Create(const D3D11_BLEND_DESC& desc)
    {
        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBlendState(&desc, m_blendState.GetAddressOf()));
    }

    const Microsoft::WRL::ComPtr<ID3D11BlendState>& BlendState::GetBlendState() const
    {
        return m_blendState;
    }
    
    ID3D11BlendState* BlendState::GetRawBlendState() const
    {
        return m_blendState.Get();
    }
}