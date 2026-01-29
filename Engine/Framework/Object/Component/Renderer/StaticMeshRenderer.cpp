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
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Resource/MaterialHelper.h"
#include "Core/Graphics/Resource/BlendState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/StaticMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Common/Utility/StaticMemoryPool.h"

namespace engine
{
    namespace
    {
        StaticMemoryPool<StaticMeshRenderer, 256> g_staticMeshRendererPool;
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
        m_pointShadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Shadow_Point_VS.hlsl");

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
        m_maskCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Mask_Cutout_PS.hlsl");
        m_pickingPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Picking_PS.hlsl");
        m_pointShadowPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Shadow_Point_PS.hlsl");
        m_pointShadowCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Shadow_Point_Cutout_PS.hlsl");

        m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
        
        switch (m_cullMode)
        {
        case CullMode::None:
            m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidNone);
            break;

        case CullMode::Back:
            m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidBack);
            break;

        case CullMode::Front:
            m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidFront);
            break;
        }

        m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));

        m_isInitialized = true;
        SystemManager::Get().GetRenderSystem().Register(this);
    }

    void StaticMeshRenderer::Update()
    {
        UpdateSockets();
    }

    void StaticMeshRenderer::SetMesh(const std::string& meshFilePath)
    {
        if (m_isInitialized)
        {
            SystemManager::Get().GetRenderSystem().Unregister(this);
        }

        m_meshFilePath = meshFilePath;

        m_staticMeshData = AssetManager::Get().GetOrCreateStaticMeshData(m_meshFilePath);
        m_materialData = AssetManager::Get().GetOrCreateMaterialData(m_meshFilePath);

        // 메쉬 로드 실패 체크
        if (!m_staticMeshData || m_staticMeshData->GetVertices().empty())
        {
            LOG_ERROR("[StaticMeshRenderer] Failed to load mesh: {}", meshFilePath);
            return;
        }

        m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(m_meshFilePath, m_staticMeshData->GetVertices());
        m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer(m_meshFilePath, m_staticMeshData->GetIndices());

        // Initialize()가 아직 호출되지 않았다면 셰이더/InputLayout 초기화
        if (!m_vs)
        {
            m_vsFilePath = "Resource/Shader/Vertex/Static_VS.hlsl";
            m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
        }
        if (!m_inputLayout)
        {
            m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
        }
        if (!m_samplerState)
        {
            m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
        }
        if (!m_objectConstantBuffer)
        {
            m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        }
        if (!m_materialConstantBuffer)
        {
            m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));
        }

        SetupTextures(m_materialData, m_textures);

        if (m_isInitialized)
        {
            SystemManager::Get().GetRenderSystem().Register(this);
        }
    }

    void StaticMeshRenderer::SetVertexShader(const std::string& shaderFilePath)
    {
        m_vsFilePath = shaderFilePath;
        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
        if (m_vs)
        {
            m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
        }
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

    void StaticMeshRenderer::SetCastShadow(bool cast)
    {
        m_castShadow = cast;
    }

    bool StaticMeshRenderer::IsCastShadow() const
    {
        return m_castShadow;
    }

    void StaticMeshRenderer::SetCullMode(CullMode cullMode)
    {
        m_cullMode = cullMode;

        switch (m_cullMode)
        {
        case CullMode::None:
            m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidNone);
            break;

        case CullMode::Back:
            m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidBack);
            break;

        case CullMode::Front:
            m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidFront);
            break;
        }
    }

    void StaticMeshRenderer::SetObstacleAlpha(bool enable, float alpha)
    {
        if (!m_isInitialized)
        {
            return; // Initialize가 안 되어있으면 무시
        }

        bool wasTransparent = m_useObstacleTransparency;
        m_useObstacleTransparency = enable;
        m_obstacleAlpha = std::clamp(alpha, 0.0f, 1.0f);

        // 상태가 변경되었을 때만 Register/Unregister
        if (wasTransparent != m_useObstacleTransparency)
        {
            SystemManager::Get().GetRenderSystem().Unregister(this);
            SystemManager::Get().GetRenderSystem().Register(this);
        }
    }

    const std::string& StaticMeshRenderer::GetMeshPath() const
    {
        return m_meshFilePath;
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

        ImGui::Checkbox("Cast Shadow", &m_castShadow);

        ImGui::SeparatorText("Material");

        ImGui::Checkbox("Override Material", &m_overrideMaterial);
        ImGui::ColorEdit4("Base Color", &m_materialBaseColor.x);
        ImGui::ColorEdit3("Emissive", &m_materialEmissive.x);
        ImGui::DragFloat("Emissive Intensity", &m_materialEmissiveIntensity, 0.01f, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Roughness", &m_materialRoughness, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Metalness", &m_materialMetalness, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Ambient Occlusion", &m_materialAmbientOcclusion, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();
        static const char* cullModes[] = { "None", "Back", "Front" };
        int currentMode = static_cast<int>(m_cullMode);
        if (ImGui::Combo("Cull mode", &currentMode, cullModes, IM_ARRAYSIZE(cullModes)))
        {
            SetCullMode(static_cast<CullMode>(currentMode));
        }

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

        DrawSocketEditor();
    }

    void StaticMeshRenderer::Save(json& j) const
    {
        Object::Save(j);

        j["MeshFilePath"] = m_meshFilePath;
        j["SocketFilePath"] = m_socketFilePath;
        j["VSFilePath"] = m_vsFilePath;
        j["OpaquePSFilePath"] = m_opaquePSFilePath;
        j["CutoutPSFilePath"] = m_cutoutPSFilePath;
        j["TransparentPSFilePath"] = m_transparentPSFilePath;
        j["MaterialBaseColor"] = m_materialBaseColor;
        j["MaterialEmissive"] = m_materialEmissive;
        j["MaterialRoughness"] = m_materialRoughness;
        j["MaterialMetalness"] = m_materialMetalness;
        j["MaterialAmbientOcclusion"] = m_materialAmbientOcclusion;
        j["MaterialEmissiveIntensity"] = m_materialEmissiveIntensity;
        j["OverrideMaterial"] = m_overrideMaterial;
        j["CastShadow"] = m_castShadow;
        j["CullMode"] = m_cullMode;
    }

    void StaticMeshRenderer::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j,"MeshFilePath", m_meshFilePath);
        JsonGet(j, "SocketFilePath", m_socketFilePath);
        JsonGet(j,"VSFilePath", m_vsFilePath);
        JsonGet(j,"OpaquePSFilePath", m_opaquePSFilePath);
        JsonGet(j,"CutoutPSFilePath", m_cutoutPSFilePath);
        JsonGet(j,"TransparentPSFilePath", m_transparentPSFilePath);
        JsonGet(j, "MaterialBaseColor", m_materialBaseColor);
        JsonGet(j, "MaterialEmissive", m_materialEmissive);
        JsonGet(j, "MaterialRoughness", m_materialRoughness);
        JsonGet(j, "MaterialMetalness", m_materialMetalness);
        JsonGet(j, "MaterialAmbientOcclusion", m_materialAmbientOcclusion);
        JsonGet(j, "MaterialEmissiveIntensity", m_materialEmissiveIntensity);
        JsonGet(j, "OverrideMaterial", m_overrideMaterial);
        JsonGet(j, "CastShadow", m_castShadow);
        JsonGet(j, "CullMode", m_cullMode);

        Refresh();
    }

    bool StaticMeshRenderer::HasRenderType(RenderType type) const
    {
        if (!m_materialData)
        {
            // 반투명 모드면 Transparent만 반환, Opaque는 반환하지 않음
            if (m_useObstacleTransparency)
            {
                return (type == RenderType::Transparent);
            }
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
        
        // 반투명 모드일 때는 Opaque와 Cutout을 렌더링하지 않음
        if (m_useObstacleTransparency)
        {
            if (type == RenderType::Transparent)
            {
                // Opaque나 Cutout 머테리얼이 있으면 반투명으로 렌더링 가능
                for (const auto& mat : m_materialData->GetMaterials())
                {
                    if (mat.renderType == MaterialRenderType::Opaque ||
                        mat.renderType == MaterialRenderType::Cutout)
                    {
                        return true;
                    }
                }
            }
            // 반투명 모드일 때는 Opaque와 Cutout은 false 반환
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
        // 리소스가 로드되지 않았으면 스킵
        if (!m_staticMeshData || !m_inputLayout || !m_vertexBuffer || !m_indexBuffer)
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
        deviceContext->RSSetState(m_rasterizerState->GetRawRasterizerState());

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
            cbMaterial.materialEmissiveIntensity = m_materialEmissiveIntensity;
            cbMaterial.materialRoughness = m_materialRoughness;
            cbMaterial.materialMetalness = m_materialMetalness;
            cbMaterial.materialAmbientOcclusion = m_materialAmbientOcclusion;
            cbMaterial.materialAlpha = m_useObstacleTransparency ? m_obstacleAlpha : 1.0f;
            cbMaterial.overrideMaterial = m_overrideMaterial ? 1 : 0;

            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
            deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);
        }

        switch (type)
        {
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
            // 투명 렌더링을 위한 state 설정
            auto blendState = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
            auto depthState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::DepthRead);
            static constexpr float blendFactor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
            deviceContext->OMSetBlendState(blendState->GetRawBlendState(), blendFactor, 0xFFFFFFFF);
            deviceContext->OMSetDepthStencilState(depthState->GetRawDepthStencilState(), 0);

            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());

            const auto& meshSections = m_staticMeshData->GetMeshSections();
            const auto& materials = m_materialData->GetMaterials();

            // 반투명 장애물 모드: Opaque/Cutout 머테리얼을 반투명으로 렌더링
            if (m_useObstacleTransparency)
            {
                // Transparent 셰이더로 Opaque/Cutout 머테리얼을 반투명 렌더링
                deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);
                
                // Opaque 머테리얼을 반투명으로 렌더링
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

                // Cutout 머테리얼을 반투명으로 렌더링
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
            else
            {
                // 일반 Transparent 머테리얼 렌더링
                deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);
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

            deviceContext->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
            deviceContext->OMSetDepthStencilState(nullptr, 0);
        }
            break;
        }
    }

    void StaticMeshRenderer::DrawShadow(RenderType renderType, LightType lightType) const
    {
        if (!m_staticMeshData || !m_inputLayout || !m_vertexBuffer || !m_indexBuffer)
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

        switch (lightType)
        {
        case LightType::Directional:
            deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);
            break;

        case LightType::Point:
            deviceContext->VSSetShader(m_pointShadowVS->GetRawShader(), nullptr, 0);
            break;
        }

        const auto& meshSections = m_staticMeshData->GetMeshSections();
        const auto& materials = m_materialData->GetMaterials();

        for (const auto& meshSection : meshSections)
        {
            switch (materials[meshSection.materialIndex].renderType)
            {
            case MaterialRenderType::Opaque:
                if (renderType != RenderType::Opaque)
                {
                    continue;
                }

                switch (lightType)
                {
                case LightType::Directional:
                    deviceContext->PSSetShader(nullptr, nullptr, 0);
                    deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
                    break;

                case LightType::Point:
                    deviceContext->PSSetShader(m_pointShadowPS->GetRawShader(), nullptr, 0);
                    deviceContext->DrawIndexedInstanced(meshSection.indexCount, 6, meshSection.indexOffset, meshSection.vertexOffset, 0);
                    break;
                }

                break;

            case MaterialRenderType::Cutout:
                if (renderType != RenderType::Cutout)
                {
                    continue;
                }

                deviceContext->PSSetShaderResources(
                    static_cast<UINT>(TextureSlot::BaseColor),
                    1,
                    m_textures[meshSection.materialIndex].baseColor->GetSRV().GetAddressOf());

                switch (lightType)
                {
                case LightType::Directional:
                    deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);
                    deviceContext->DrawIndexed(meshSection.indexCount, meshSection.indexOffset, meshSection.vertexOffset);
                    break;

                case LightType::Point:
                    deviceContext->PSSetShader(m_pointShadowCutoutPS->GetRawShader(), nullptr, 0);
                    deviceContext->DrawIndexedInstanced(meshSection.indexCount, 6, meshSection.indexOffset, meshSection.vertexOffset, 0);
                    break;
                }

                break;

            default:
                continue;
            }
        }

    }

    DirectX::BoundingBox StaticMeshRenderer::GetBounds() const
    {
        return DirectX::BoundingBox();
    }

    void StaticMeshRenderer::DrawMask() const
    {
        if (!m_staticMeshData || !m_inputLayout || !m_vertexBuffer || !m_indexBuffer)
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
        if (!m_staticMeshData || !m_inputLayout || !m_vertexBuffer || !m_indexBuffer)
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

    void StaticMeshRenderer::UpdateSockets()
    {
        auto gameObject = GetGameObject();
        if (!gameObject) return;

        auto transform = GetTransform();
        if (!transform) return;

        for (auto& instance : m_socketInstances)
        {
            if (!instance.info) continue;

            instance.worldMatrix = instance.info->localMatrix * transform->GetWorld();
        }

        for (auto& attached : m_attachedObjects)
        {
            if (!attached.obj) continue;

            Matrix socketWorld = GetSocketWorldMatrix(attached.socketName);
            attached.obj->GetTransform()->SetWorldMatrix(socketWorld);
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

        if (!m_socketFilePath.empty() || !m_meshFilePath.empty())
        {
            LoadSocketData();
        }

        SystemManager::Get().GetRenderSystem().Register(this);
    }
}