#include "EnginePCH.h"
#include "VertexBuffer.h"

namespace engine
{
    const Microsoft::WRL::ComPtr<ID3D11Buffer>& VertexBuffer::GetBuffer() const
    {
        return m_buffer;
    }

    ID3D11Buffer* VertexBuffer::GetRawBuffer() const
    {
        return m_buffer.Get();
    }

    UINT VertexBuffer::GetBufferStride() const
    {
        return m_bufferStride;
    }

    UINT VertexBuffer::GetVertexCount() const
    {
        return m_vertexCount;
    }
}