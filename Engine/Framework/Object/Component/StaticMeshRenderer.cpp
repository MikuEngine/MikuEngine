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
        m_worldTransformBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("WorldTransform", sizeof(WorldTransformBuffer));
        m_finalPassVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/BasicVS.hlsl");
        m_shadowPassVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/BasicLightViewVS.hlsl");
        m_finalPassPixelShader = ResourceManager::Get().GetOrCreatePixelShader(shaderFilePath);
        m_shadowPassPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Shader/Pixel/LightViewPS.hlsl");
        m_inputLayout = m_finalPassVertexShader->GetOrCreateInputLayout<CommonVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
        m_comparisonSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Comparison);

        SetupTextures(m_materialData, m_textures);
    }

    StaticMeshRenderer::StaticMeshRenderer(DefaultStaticMeshType type, const std::string& shaderFilePath)
    {
        SystemManager::Get().Render().Register(this);

        m_staticMeshData = AssetManager::Get().GetDefaultStaticMeshData(type);
        m_materialData = std::make_shared<MaterialData>();
        m_materialData->Create();

        m_vertexBuffer = ResourceManager::Get().GetDefaultVertexBuffer(type);
        m_indexBuffer = ResourceManager::Get().GetDefaultIndexBuffer(type);
        m_worldTransformBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("WorldTransform", sizeof(WorldTransformBuffer));
        m_finalPassVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/BasicVS.hlsl");
        m_shadowPassVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/BasicLightViewVS.hlsl");
        m_finalPassPixelShader = ResourceManager::Get().GetOrCreatePixelShader(shaderFilePath);
        m_shadowPassPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Shader/Pixel/LightViewPS.hlsl");
        m_inputLayout = m_finalPassVertexShader->GetOrCreateInputLayout<CommonVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
        m_comparisonSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Comparison);

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
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::WorldTransform),
            1, m_worldTransformBuffer->GetBuffer().GetAddressOf());
        
        WorldTransformBuffer worldData{};
        worldData.world = GetTransform()->GetWorld().Transpose();
        deviceContext->UpdateSubresource(m_worldTransformBuffer->GetRawBuffer(), 0, nullptr, &worldData, 0, 0);

        deviceContext->PSSetSamplers(0, 1, m_samplerState->GetSamplerState().GetAddressOf());
        deviceContext->PSSetSamplers(1, 1, m_comparisonSamplerState->GetSamplerState().GetAddressOf());
        deviceContext->PSSetShader(m_finalPassPixelShader->GetRawShader(), nullptr, 0);

        //deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material),
        //    1, m_materialBuffer->GetBuffer().GetAddressOf());

        const auto& meshSections = m_staticMeshData->GetMeshSections();

        for (const auto& meshSection : meshSections)
        {
            const auto textureSRVs = m_textures[meshSection.materialIndex].AsRawArray();

            deviceContext->PSSetShaderResources(0, static_cast<UINT>(textureSRVs.size()), textureSRVs.data());
            //deviceContext->UpdateSubresource(m_materialBuffer->GetRawBuffer(), 0, nullptr, &m_materialCBs[meshSection.materialIndex], 0, 0);
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