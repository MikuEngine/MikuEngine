#include "EnginePCH.h"
#include "IndexBuffer.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    void IndexBuffer::Create(const std::vector<DWORD>& indices)
    {
        m_indexFormat = DXGI_FORMAT_R32_UINT;
        m_indexCount = static_cast<UINT>(indices.size());

        D3D11_BUFFER_DESC indexBufferDesc{};
        indexBufferDesc.ByteWidth = sizeof(DWORD) * m_indexCount;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

        D3D11_SUBRESOURCE_DATA indexBufferData{};
        indexBufferData.pSysMem = indices.data();

        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_buffer));
    }

    void IndexBuffer::Create(const std::vector<WORD>& indices)
    {
        m_indexFormat = DXGI_FORMAT_R16_UINT;
        m_indexCount = static_cast<UINT>(indices.size());

        D3D11_BUFFER_DESC indexBufferDesc{};
        indexBufferDesc.ByteWidth = sizeof(WORD) * m_indexCount;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

        D3D11_SUBRESOURCE_DATA indexBufferData{};
        indexBufferData.pSysMem = indices.data();

        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_buffer));
    }

    const Microsoft::WRL::ComPtr<ID3D11Buffer>& IndexBuffer::GetBuffer() const
    {
        return m_buffer;
    }

    ID3D11Buffer* IndexBuffer::GetRawBuffer() const
    {
        return m_buffer.Get();
    }

    DXGI_FORMAT IndexBuffer::GetIndexFormat() const
    {
        return m_indexFormat;
    }

    UINT IndexBuffer::GetIndexCount() const
    {
        return m_indexCount;
    }
}