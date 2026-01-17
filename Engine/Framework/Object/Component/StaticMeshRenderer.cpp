#include "EnginePCH.h"
#include "StaticMeshRenderer.h"

#include <filesystem>

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/MaterialHelper.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/StaticMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Transform.h"
#include "Common/Utility/StaticMemoryPool.h"

namespace engine
{
    namespace
    {
        StaticMemoryPool<StaticMeshRenderer, 1024> g_staticMeshRendererPool;
    }

    StaticMeshRenderer::~StaticMeshRenderer()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);
    }

    void* StaticMeshRenderer::operator new(size_t size)
    {
        return g_staticMeshRendererPool.Allocate(size);
    }

    void StaticMeshRenderer::operator delete(void* ptr)
    {
        g_staticMeshRendererPool.Deallocate(ptr);
    }

    void StaticMeshRenderer::Initialize()
    {
        m_vsFilePath = "Resource/Shader/Vertex/Static_VS.hlsl";
        m_opaquePSFilePath = "Resource/Shader/Pixel/GBuffer_PS.hlsl";
        m_cutoutPSFilePath = "Resource/Shader/Pixel/GBuffer_Cutout_PS.hlsl";
        m_transparentPSFilePath = "Resource/Shader/Pixel/LightTransparent_PS.hlsl";

        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
        m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Shadow_Static_VS.hlsl");
        m_simpleVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Simple_Static_VS.hlsl");

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
        m_maskCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Mask_Cutout_PS.hlsl");
        m_pickingPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Picking_PS.hlsl");

        m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

        m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));
    }

    void StaticMeshRenderer::SetMesh(const std::string& meshFilePath)
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);

        m_meshFilePath = meshFilePath;

        m_staticMeshData = AssetManager::Get().GetOrCreateStaticMeshData(m_meshFilePath);
        m_materialData = AssetManager::Get().GetOrCreateMaterialData(m_meshFilePath);

        m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(m_meshFilePath, m_staticMeshData->GetVertices());
        m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer(m_meshFilePath, m_staticMeshData->GetIndices());

        SetupTextures(m_materialData, m_textures);

        SystemManager::Get().GetRenderSystem().Register(this);
    }

    void StaticMeshRenderer::SetVertexShader(const std::string& shaderFilePath)
    {
        m_vsFilePath = shaderFilePath;
        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
    }

    void StaticMeshRenderer::SetOpaquePixelShader(const std::string& shaderFilePath)
    {
        m_opaquePSFilePath = shaderFilePath;
        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
    }

    void StaticMeshRenderer::SetCutoutPixelShader(const std::string& shaderFilePath)
    {
        m_cutoutPSFilePath = shaderFilePath;
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
    }

    void StaticMeshRenderer::SetTransparentPixelShader(const std::string& shaderFilePath)
    {
        m_transparentPSFilePath = shaderFilePath;
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
    }

    void StaticMeshRenderer::OnGui()
    {
        ImGui::Text("Mesh: %s", m_meshFilePath.c_str());
        std::string selectedMesh;
        // 경로 주의: 실행 파일 기준 경로 (보통 Resource/Model)
        if (DrawFileSelector("Select Mesh (.fbx)", "Resource/Model", ".fbx", selectedMesh))
        {
            SetMesh(selectedMesh);
        }

        ImGui::SeparatorText("Material");

        ImGui::Checkbox("Override Material", &m_overrideMaterial);
        ImGui::ColorEdit4("Base Color", &m_materialBaseColor.x);
        ImGui::ColorEdit3("Emissive", &m_materialEmissive.x);
        ImGui::DragFloat("Roughness", &m_materialRoughness, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Metalness", &m_materialMetalness, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Ambient Occlusion", &m_materialAmbientOcclusion, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();
        // 2. Shader Selectors
        // (Shader 폴더 경로가 Resource/Shader인지 Shader인지 확인 필요)
        static const std::string pixelShaderPath = "Resource/Shader/Pixel";
        static const std::string vertexShaderPath = "Resource/Shader/Vertex";
        ImGui::Text("Shaders:");
        std::string selectedShader;
        // Opaque
        if (DrawFileSelector("VS", vertexShaderPath, ".hlsl", selectedShader))
        {
            SetVertexShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_vsFilePath).filename().string().c_str());
        // Opaque
        if (DrawFileSelector("Opaque PS", pixelShaderPath, ".hlsl", selectedShader))
        {
            SetOpaquePixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_opaquePSFilePath).filename().string().c_str());
        // Cutout
        if (DrawFileSelector("Cutout PS", pixelShaderPath, ".hlsl", selectedShader))
        {
            SetCutoutPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_cutoutPSFilePath).filename().string().c_str());
        // Transparent
        if (DrawFileSelector("Trans PS", pixelShaderPath, ".hlsl", selectedShader))
        {
            SetTransparentPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_transparentPSFilePath).filename().string().c_str());
        // 3. Material Info Visualization
        if (m_materialData)
        {
            ImGui::Separator();
            if (ImGui::TreeNode("Materials Info"))
            {
                int i = 0;
                for (const auto& mat : m_materialData->GetMaterials())
                {
                    ImGui::PushID(i++);
                    ImGui::Text("[%d] Type: %d", i - 1, (int)mat.renderType);
                    if (mat.texturePaths.count(MaterialKey::BASE_COLOR_TEXTURE))
                    {
                        std::string tex = std::filesystem::path(mat.texturePaths.at(MaterialKey::BASE_COLOR_TEXTURE)).filename().string();
                        ImGui::BulletText("Tex: %s", tex.c_str());
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
    }

    void StaticMeshRenderer::Save(json& j) const
    {
        Object::Save(j);

        j["MeshFilePath"] = m_meshFilePath;
        j["VSFilePath"] = m_vsFilePath;
        j["OpaquePSFilePath"] = m_opaquePSFilePath;
        j["CutoutPSFilePath"] = m_cutoutPSFilePath;
        j["TransparentPSFilePath"] = m_transparentPSFilePath;
        j["MaterialBaseColor"] = m_materialBaseColor;
        j["MaterialEmissive"] = m_materialEmissive;
        j["MaterialRoughness"] = m_materialRoughness;
        j["MaterialMetalness"] = m_materialMetalness;
        j["MaterialAmbientOcclusion"] = m_materialAmbientOcclusion;
        j["OverrideMaterial"] = m_overrideMaterial;
    }

    void StaticMeshRenderer::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j,"MeshFilePath", m_meshFilePath);
        JsonGet(j,"VSFilePath", m_vsFilePath);
        JsonGet(j,"OpaquePSFilePath", m_opaquePSFilePath);
        JsonGet(j,"CutoutPSFilePath", m_cutoutPSFilePath);
        JsonGet(j,"TransparentPSFilePath", m_transparentPSFilePath);
        JsonGet(j, "MaterialBaseColor", m_materialBaseColor);
        JsonGet(j, "MaterialEmissive", m_materialEmissive);
        JsonGet(j, "MaterialRoughness", m_materialRoughness);
        JsonGet(j, "MaterialMetalness", m_materialMetalness);
        JsonGet(j, "MaterialAmbientOcclusion", m_materialAmbientOcclusion);
        JsonGet(j, "OverrideMaterial", m_overrideMaterial);

        Refresh();
    }

    std::string StaticMeshRenderer::GetType() const
    {
        return "StaticMeshRenderer";
    }

    bool StaticMeshRenderer::HasRenderType(RenderType type) const
    {
        if (!m_materialData)
        {
            return (type == RenderType::Opaque);
        }
        
        if (type == RenderType::Shadow)
        {
            for (const auto& mat : m_materialData->GetMaterials())
            {
                if (mat.renderType == MaterialRenderType::Opaque ||
                    mat.renderType == MaterialRenderType::Cutout)
                {
                    return true;
                }
            }

            return false;
        }
        
        MaterialRenderType targetMatType;
        switch (type)
        {
        case RenderType::Opaque:
            targetMatType = MaterialRenderType::Opaque;
            break;
        case RenderType::Cutout:
            targetMatType = MaterialRenderType::Cutout;
            break;
        case RenderType::Transparent:
            targetMatType = MaterialRenderType::Transparent;
            break;
        default:
            return false; // Screen 등 나머지는 지원 안 함
        }
        
        for (const auto& mat : m_materialData->GetMaterials())
        {
            if (mat.renderType == targetMatType)
            {
                return true;
            }
        }
        return false;
    }

    void StaticMeshRenderer::Draw(RenderType type) const
    {
        if (!m_staticMeshData)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride();

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();
        cbObject.worldInverseTranspose = GetTransform()->GetWorld().Invert();

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object),
            1, m_objectConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        if (type != RenderType::Shadow)
        {
            CbMaterial cbMaterial{};
            cbMaterial.materialBaseColor = m_materialBaseColor;
            cbMaterial.materialEmissive = m_materialEmissive;
            cbMaterial.materialRoughness = m_materialRoughness;
            cbMaterial.materialMetalness = m_materialMetalness;
            cbMaterial.materialAmbientOcclusion = m_materialAmbientOcclusion;
            cbMaterial.overrideMaterial = m_overrideMaterial ? 1 : 0;

            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
            deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);
        }

        switch (type)
        {
        case RenderType::Shadow:
        {
            deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);

            const auto& meshSections = m_staticMeshData->GetMeshSections();
            const auto& materials = m_materialData->GetMaterials();

            for (const auto& meshSection : meshSections)
            {
                switch (materials[meshSection.materialIndex].renderType)
                {
                case MaterialRenderType::Opaque:
                    deviceContext->PSSetShader(nullptr, nullptr, 0);
                    deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);

                    break;

                case MaterialRenderType::Cutout:
                    deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);
                    deviceContext->PSSetShaderResources(
                        static_cast<UINT>(TextureSlot::BaseColor),
                        1,
                        m_textures[meshSection.materialIndex].baseColor->GetSRV().GetAddressOf());
                    deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
                    break;

                default:
                    continue;
                }
            }
        }
            break;

        case RenderType::Opaque:
        {
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
            deviceContext->PSSetShader(m_opaquePS->GetRawShader(), nullptr, 0);

            const auto& meshSections = m_staticMeshData->GetMeshSections();
            const auto& materials = m_materialData->GetMaterials();



            for (const auto& meshSection : meshSections)
            {
                if (materials[meshSection.materialIndex].renderType != MaterialRenderType::Opaque)
                {
                    continue;
                }

                const auto textureSRVs = m_textures[meshSection.materialIndex].AsRawArray();

                deviceContext->PSSetShaderResources(
                    static_cast<UINT>(TextureSlot::BaseColor),
                    static_cast<UINT>(textureSRVs.size()),
                    textureSRVs.data());
                deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
            }
        }
            break;

        case RenderType::Cutout:
        {
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
            deviceContext->PSSetShader(m_cutoutPS->GetRawShader(), nullptr, 0);

            const auto& meshSections = m_staticMeshData->GetMeshSections();
            const auto& materials = m_materialData->GetMaterials();

            for (const auto& meshSection : meshSections)
            {
                if (materials[meshSection.materialIndex].renderType != MaterialRenderType::Cutout)
                {
                    continue;
                }

                const auto textureSRVs = m_textures[meshSection.materialIndex].AsRawArray();

                deviceContext->PSSetShaderResources(
                    static_cast<UINT>(TextureSlot::BaseColor),
                    static_cast<UINT>(textureSRVs.size()),
                    textureSRVs.data());

                deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
            }
        }
            break;

        case RenderType::Transparent:
        {
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
            deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);

            const auto& meshSections = m_staticMeshData->GetMeshSections();
            const auto& materials = m_materialData->GetMaterials();

            for (const auto& meshSection : meshSections)
            {
                if (materials[meshSection.materialIndex].renderType != MaterialRenderType::Transparent)
                {
                    continue;
                }

                const auto textureSRVs = m_textures[meshSection.materialIndex].AsRawArray();

                deviceContext->PSSetShaderResources(
                    static_cast<UINT>(TextureSlot::BaseColor),
                    static_cast<UINT>(textureSRVs.size()),
                    textureSRVs.data());

                deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
            }
        }
            break;
        }
    }

    DirectX::BoundingBox StaticMeshRenderer::GetBounds() const
    {
        return DirectX::BoundingBox();
    }

    void StaticMeshRenderer::DrawMask() const
    {
        if (!m_staticMeshData)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride();

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object),
            1, m_objectConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->VSSetShader(m_simpleVS->GetRawShader(), nullptr, 0);
        deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);

        const auto& meshSections = m_staticMeshData->GetMeshSections();
        const auto& materials = m_materialData->GetMaterials();

        for (const auto& meshSection : meshSections)
        {
            ID3D11ShaderResourceView* srv = m_textures[meshSection.materialIndex].baseColor->GetRawSRV();
            deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);
            deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
        }
    }

    void StaticMeshRenderer::DrawPickingID() const
    {
        if (!m_staticMeshData)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride();

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object),
            1, m_objectConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->VSSetShader(m_simpleVS->GetRawShader(), nullptr, 0);
        deviceContext->PSSetShader(m_pickingPS->GetRawShader(), nullptr, 0);

        const auto& meshSections = m_staticMeshData->GetMeshSections();
        const auto& materials = m_materialData->GetMaterials();

        for (const auto& meshSection : meshSections)
        {
            ID3D11ShaderResourceView* srv = m_textures[meshSection.materialIndex].baseColor->GetRawSRV();
            deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);
            deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
        }
    }

    void StaticMeshRenderer::Refresh()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);

        m_staticMeshData = AssetManager::Get().GetOrCreateStaticMeshData(m_meshFilePath);
        m_materialData = AssetManager::Get().GetOrCreateMaterialData(m_meshFilePath);

        m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(m_meshFilePath, m_staticMeshData->GetVertices());
        m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer(m_meshFilePath, m_staticMeshData->GetIndices());

        SetupTextures(m_materialData, m_textures);

        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);

        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);

        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);

        SystemManager::Get().GetRenderSystem().Register(this);
    }
}