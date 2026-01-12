#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
    class IndexBuffer :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
        DXGI_FORMAT m_indexFormat = DXGI_FORMAT_UNKNOWN;
        UINT m_indexCount = 0;

    public:
        void Create(const std::vector<DWORD>& indices);
        void Create(const std::vector<WORD>& indices);

    public:
        const Microsoft::WRL::ComPtr<ID3D11Buffer>& GetBuffer() const;
        ID3D11Buffer* GetRawBuffer() const;
        DXGI_FORMAT GetIndexFormat() const;
        UINT GetIndexCount() const;
    };
}