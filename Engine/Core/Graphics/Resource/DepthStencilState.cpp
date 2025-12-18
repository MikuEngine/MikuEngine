#include "pch.h"
#include "DepthStencilState.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    void DepthStencilState::Create(const D3D11_DEPTH_STENCIL_DESC& desc)
    {
        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateDepthStencilState(&desc, m_depthStencilState.GetAddressOf()));
    }

    const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& DepthStencilState::GetDepthStencilState() const
    {
        return m_depthStencilState;
    }

    ID3D11DepthStencilState* DepthStencilState::GetRawDepthStencilState() const
    {
        return m_depthStencilState.Get();
    }
}