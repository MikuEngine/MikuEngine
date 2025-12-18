#pragma once

#include <vector>

namespace engine
{
    struct Vertex
    {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector2 texCoord;
        DirectX::SimpleMath::Vector3 normal;
    };

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        void Initialize(const Microsoft::WRL::ComPtr<ID3D11Device>& device, const MeshData& meshData);
        void Render(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext);

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        UINT m_vertexCount = 0;
        UINT m_indexCount = 0;
        UINT m_stride = 0;
        UINT m_offset = 0;
    };
}
