#include "pch.h"
#include "Mesh.h"

namespace engine
{
    void Mesh::Initialize(const Microsoft::WRL::ComPtr<ID3D11Device>& device, const MeshData& meshData)
    {
        m_vertexCount = static_cast<UINT>(meshData.vertices.size());
        m_indexCount = static_cast<UINT>(meshData.indices.size());
        m_stride = sizeof(Vertex);
        m_offset = 0;

        D3D11_BUFFER_DESC vertexBufferDesc{};
        vertexBufferDesc.ByteWidth = sizeof(Vertex) * m_vertexCount;
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vertexData{};
        vertexData.pSysMem = meshData.vertices.data();
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer));

        D3D11_BUFFER_DESC indexBufferDesc{};
        indexBufferDesc.ByteWidth = sizeof(uint32_t) * m_indexCount;
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA indexData{};
        indexData.pSysMem = meshData.indices.data();
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer));
    }

    void Mesh::Render(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext)
    {
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &m_stride, &m_offset);
        deviceContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->DrawIndexed(m_indexCount, 0, 0);
    }
}
