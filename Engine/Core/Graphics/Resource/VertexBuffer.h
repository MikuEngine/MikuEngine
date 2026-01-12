#pragma once

#include "Core/Graphics/Resource/Resource.h"
#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    class VertexBuffer :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
        UINT m_bufferStride = 0;
        UINT m_vertexCount = 0;

    public:
        template <typename T>
        void Create(const std::vector<T>& vertices)
        {
            m_bufferStride = sizeof(T);
            m_vertexCount = static_cast<UINT>(vertices.size());

            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = m_bufferStride * m_vertexCount;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = vertices.data();

            HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&desc, &data, &m_buffer));
        }

    public:
        const Microsoft::WRL::ComPtr<ID3D11Buffer>& GetBuffer() const;
        ID3D11Buffer* GetRawBuffer() const;
        UINT GetBufferStride() const;
        UINT GetVertexCount() const;
    };
}