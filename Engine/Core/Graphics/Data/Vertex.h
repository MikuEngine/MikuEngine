#pragma once

#include <d3d11.h>
#include <directxtk/SimpleMath.h> 
#include <array>

namespace engine
{
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;

    enum class VertexFormat
    {
        Common,
        Position,
        PositionNormal,
        PositionColor,
        PositionTexCoord,
        BoneWeight
    };

    struct CommonVertex
    {
        Vector3 position;
        Vector2 texCoord;
        Vector3 normal;
        Vector3 tangent;
        Vector3 binormal;

        CommonVertex() noexcept = default;
        
        CommonVertex(
            const float* position,
            const float* texCoord,
            const float* normal,
            const float* tangent,
            const float* binormal) noexcept
            : position{ position },
            texCoord{ texCoord },
            normal{ normal },
            tangent{ tangent },
            binormal{ binormal }
        {

        }

        CommonVertex(
            const Vector3& position,
            const Vector2& texCoord,
            const Vector3& normal,
            const Vector3& tangent,
            const Vector3& binormal) noexcept
            : position{ position },
            texCoord{ texCoord },
            normal{ normal },
            tangent{ tangent },
            binormal{ binormal }
        {

        }

        static constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 5> layout
        {
            // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        static constexpr VertexFormat vertexFormat = VertexFormat::Common;
    };

    struct PositionVertex
    {
        Vector3 position;

        static constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> layout
        {
            // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        static constexpr VertexFormat vertexFormat = VertexFormat::Position;
    };

    struct PositionNormalVertex
    {
        Vector3 position;
        Vector3 normal;

        static constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> layout
        {
            // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        static constexpr VertexFormat vertexFormat = VertexFormat::PositionNormal;
    };

    struct PositionColorVertex
    {
        Vector3 position;
        Vector4 color;

        static constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> layout
        {
            // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "COLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        static constexpr VertexFormat vertexFormat = VertexFormat::PositionColor;
    };

    struct PositionTexCoordVertex
    {
        Vector3 position;
        Vector2 texCoord;

        static constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> layout
        {
            // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        static constexpr VertexFormat vertexFormat = VertexFormat::PositionTexCoord;
    };

    struct BoneWeightVertex
    {
        Vector3 position;
        Vector2 texCoord;
        Vector3 normal;
        Vector3 tangent;
        Vector3 binormal;
        unsigned int blendIndices[4]{};
        float blendWeights[4]{};

        BoneWeightVertex() noexcept = default;

        BoneWeightVertex(
            const float* position,
            const float* texCoord,
            const float* normal,
            const float* tangent,
            const float* binormal) noexcept
            : position{ position },
            texCoord{ texCoord },
            normal{ normal },
            tangent{ tangent },
            binormal{ binormal }
        {

        }

        BoneWeightVertex(
            const Vector3& position,
            const Vector2& texCoord,
            const Vector3& normal,
            const Vector3& tangent,
            const Vector3& binormal) noexcept
            : position{ position },
            texCoord{ texCoord },
            normal{ normal },
            tangent{ tangent },
            binormal{ binormal }
        {

        }

        void AddBoneData(unsigned int boneIndex, float weight)
        {
            assert(blendWeights[0] == 0.0f ||
                blendWeights[1] == 0.0f ||
                blendWeights[2] == 0.0f ||
                blendWeights[3] == 0.0f);

            for (int i = 0; i < 4; ++i)
            {
                if (blendWeights[i] == 0.0f)
                {
                    blendIndices[i] = boneIndex;
                    blendWeights[i] = weight;

                    return;
                }
            }
        }

        static constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 7> layout
        {
            // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate
            D3D11_INPUT_ELEMENT_DESC{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "BINORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        static constexpr VertexFormat vertexFormat = VertexFormat::BoneWeight;
    };
}