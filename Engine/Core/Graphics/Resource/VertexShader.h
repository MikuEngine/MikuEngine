#pragma once

#include "Core/Graphics/Resource/Resource.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Data/Vertex.h"

namespace engine
{
    class VertexShader :
        public Resource
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShaderBuffer;
        std::unordered_map<VertexFormat, std::shared_ptr<InputLayout>> m_inputLayouts;

    public:
        void Create(const std::string& filePath);

        template <typename T>
        void GetOrCreateInputLayout()
        {
            VertexFormat format = T::Format;

            if (auto it = m_inputLayouts.find(format); it != m_inputLayouts.end())
            {
                return it->second;
            }

            auto layoutDesc = T::GetLayout();
            
            auto inputLayout = std::make_shared<InputLayout>();
            inputLayout->Create(m_vertexShaderBuffer, layoutDesc.data, layoutDesc.size());

            m_inputLayouts[format] = inputLayout;

            return inputLayout;
        }

    public:
        const Microsoft::WRL::ComPtr<ID3D11VertexShader>& GetShader() const;
        ID3D11VertexShader* GetRawShader() const;
    };
}