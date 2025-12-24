#include "pch.h"
#include "StaticMeshRenderer.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Common/Utility/MaterialHelper.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/StaticMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"


namespace engine
{
    StaticMeshRenderer::StaticMeshRenderer()
    {
        SystemManager::Get().Render().Register(this);
    }

    StaticMeshRenderer::StaticMeshRenderer(const std::string& meshFilePath, const std::string& shaderFilePath)
    {
        SystemManager::Get().Render().Register(this);

        m_staticMeshData = AssetManager::Get().GetOrCreateStaticMeshData(meshFilePath);
        m_materialData = AssetManager::Get().GetOrCreateMaterialData(meshFilePath);

        m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(meshFilePath, m_staticMeshData->GetVertices());
        m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer(meshFilePath, m_staticMeshData->GetIndices());

        m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));

        m_finalPassVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/Static_VS.hlsl");
        m_shadowPassVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/Shadow_Static_VS.hlsl");
        m_finalPassPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Shader/Pixel/GBuffer_PS.hlsl");
        m_shadowPassPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Shader/Pixel/Shadow_AlphaTest_PS.hlsl");

        m_inputLayout = m_finalPassVertexShader->GetOrCreateInputLayout<CommonVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

        SetupTextures(m_materialData, m_textures);
    }

    StaticMeshRenderer::~StaticMeshRenderer()
    {
        SystemManager::Get().Render().Unregister(this);
    }

    void StaticMeshRenderer::SetMesh(const std::string& meshFilePath)
    {
    }

    void StaticMeshRenderer::SetPixelShader(const std::string& shaderFilePath)
    {
    }

    bool StaticMeshRenderer::HasRenderType(RenderType type) const
    {
        return RenderType::Opaque == type;
    }

    void StaticMeshRenderer::Draw() const
    {
        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride();

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        deviceContext->VSSetShader(m_finalPassVertexShader->GetRawShader(), nullptr, 0);
        
        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();
        cbObject.worldInverseTranspose = GetTransform()->GetWorld().Invert();

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object),
            1, m_objectConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);

        deviceContext->PSSetShader(m_finalPassPixelShader->GetRawShader(), nullptr, 0);
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());

        const auto& meshSections = m_staticMeshData->GetMeshSections();

        for (const auto& meshSection : meshSections)
        {
            const auto textureSRVs = m_textures[meshSection.materialIndex].AsRawArray();

            deviceContext->PSSetShaderResources(0, static_cast<UINT>(textureSRVs.size()), textureSRVs.data());

            CbMaterial cbMaterial{};
            cbMaterial.materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
            cbMaterial.materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
            cbMaterial.materialRoughness = 0.0f;
            cbMaterial.materialMetalness = 1.0f;
            cbMaterial.materialAmbientOcclusion = 0.7f;
            cbMaterial.overrideMaterial = 0;

            deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);
            deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
        }
    }

    DirectX::BoundingBox StaticMeshRenderer::GetBounds() const
    {
        return DirectX::BoundingBox();
    }

    //void StaticMeshRenderer::DrawShadow() const
    //{
    //}
}