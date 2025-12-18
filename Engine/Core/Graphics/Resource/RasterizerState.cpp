#include "pch.h"
#include "RasterizerState.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    void RasterizerState::Create(const D3D11_RASTERIZER_DESC& desc)
    {
        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateRasterizerState(&desc, m_rasterizerState.GetAddressOf()));
    }

    const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& RasterizerState::GetRasterizerState() const
    {
        return m_rasterizerState;
    }

    ID3D11RasterizerState* RasterizerState::GetRawRasterizerState() const
    {
        return m_rasterizerState.Get();
    }
}