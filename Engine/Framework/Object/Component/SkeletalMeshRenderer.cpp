#include "EnginePCH.h"
#include "SkeletalMeshRenderer.h"

#include <filesystem>
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/SkeletalMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/Asset/SkeletonData.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/MaterialHelper.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/SkeletalAnimator.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Common/Utility/StaticMemoryPool.h"

namespace engine
{
    namespace
    {
        StaticMemoryPool<SkeletalMeshRenderer, 1024> g_skeletalMeshRendererPool;
    }

    SkeletalMeshRenderer::SkeletalMeshRenderer() = default;

    SkeletalMeshRenderer::~SkeletalMeshRenderer()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);
    }

    void* SkeletalMeshRenderer::operator new(size_t size)
    {
        return g_skeletalMeshRendererPool.Allocate(size);
    }

    void SkeletalMeshRenderer::operator delete(void* ptr)
    {
        g_skeletalMeshRendererPool.Deallocate(ptr);
    }

    void SkeletalMeshRenderer::Initialize()
    {
        // 셰이더 경로 기본값 설정
        // Skinned VS / GBuffer PS
        m_vsFilePath = "Resource/Shader/Vertex/Skinned_VS.hlsl"; 
        m_opaquePSFilePath = "Resource/Shader/Pixel/GBuffer_PS.hlsl";
        m_cutoutPSFilePath = "Resource/Shader/Pixel/GBuffer_Cutout_PS.hlsl";
        m_transparentPSFilePath = "Resource/Shader/Pixel/LightTransparent_PS.hlsl";

        m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        m_boneConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Bone", sizeof(CbBone));
        m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));

        // T-Pose 초기화
        for (auto& m : m_boneTransformData.boneTransform)
        {
            m = Matrix::Identity;
        }

        // 샘플러
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

        // 셰이더 로드 (기본)
        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
        m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Shadow_Skinned_VS.hlsl"); // Skinned Shadow VS
        m_simpleVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Simple_Skinned_VS.hlsl");

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
        m_maskCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Mask_Cutout_PS.hlsl");
        m_pickingPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Picking_PS.hlsl");
    }

    void SkeletalMeshRenderer::Awake()
    {

    }

    void SkeletalMeshRenderer::Refresh()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);

        m_meshData = AssetManager::Get().GetOrCreateSkeletalMeshData(m_meshFilePath);
        
        m_materialData = AssetManager::Get().GetOrCreateMaterialData(m_meshFilePath);
        
        m_skeletonData = AssetManager::Get().GetOrCreateSkeletonData(m_meshFilePath);

        // Vertex/Index Buffer 생성
        if (m_meshData)
        {
            if (m_meshData->IsRigid())
            {
                // Rigid Mesh (CommonVertex)
                m_vsFilePath = "Resource/Shader/Vertex/Rigid_VS.hlsl";
                m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
                m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Shadow_Rigid_VS.hlsl");
                m_simpleVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Simple_Rigid_VS.hlsl");

                m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(m_meshFilePath, m_meshData->GetVertices());
                
                m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
            }
            else
            {
                // Skinned Mesh (BoneWeightVertex)
                m_vsFilePath = "Resource/Shader/Vertex/Skinned_VS.hlsl";
                m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
                m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Shadow_Skinned_VS.hlsl");
                m_simpleVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Simple_Skinned_VS.hlsl");

                m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<BoneWeightVertex>(m_meshFilePath, m_meshData->GetBoneWeightVertices());
                
                 m_inputLayout = m_vs->GetOrCreateInputLayout<BoneWeightVertex>();
            }

            m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer(m_meshFilePath, m_meshData->GetIndices());
        }

        // 텍스처 로드
        SetupTextures(m_materialData, m_textures);

        SystemManager::Get().GetRenderSystem().Register(this);
    }

    void SkeletalMeshRenderer::SetMesh(const std::string& meshName)
    {
        m_meshFilePath = meshName;
        Refresh();
    }
    
    // Shader Setters
    void SkeletalMeshRenderer::SetVertexShader(const std::string& shaderFilePath) 
    {
        m_vsFilePath = shaderFilePath;
        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
    }

    void SkeletalMeshRenderer::SetOpaquePixelShader(const std::string& shaderFilePath) 
    {
        m_opaquePSFilePath = shaderFilePath;
        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
    }

    void SkeletalMeshRenderer::SetCutoutPixelShader(const std::string& shaderFilePath) 
    {
        m_cutoutPSFilePath = shaderFilePath;
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
    }

    void SkeletalMeshRenderer::SetTransparentPixelShader(const std::string& shaderFilePath) 
    {
        m_transparentPSFilePath = shaderFilePath;
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
    }

    std::shared_ptr<SkeletonData> SkeletalMeshRenderer::GetSkeletonData() const
    {
        return m_skeletonData;
    }

    const std::string& SkeletalMeshRenderer::GetMeshPath() const
    {
        return m_meshFilePath;
    }

    std::string SkeletalMeshRenderer::GetType() const
    {
        return "SkeletalMeshRenderer";
    }

    void SkeletalMeshRenderer::Update()
    {
        // Animator에서 행렬 가져오기
        auto animator = GetGameObject()->GetComponent<SkeletalAnimator>();
        if (animator)
        {
            std::memcpy(
                m_boneTransformData.boneTransform,
                animator->GetFinalBoneMatrices().data(),
                sizeof(Matrix) * 128);
        }
        else
        {
            // 애니메이터 없으면 Identity?
            // (Initialize에서 이미 초기화됨)
        }
    }

    bool SkeletalMeshRenderer::HasRenderType(RenderType type) const
    {
        if (!m_materialData) return (type == RenderType::Opaque);

        if (type == RenderType::Shadow)
        {
            for (const auto& mat : m_materialData->GetMaterials())
            {
                if (mat.renderType == MaterialRenderType::Opaque || mat.renderType == MaterialRenderType::Cutout)
                    return true;
            }
            return false;
        }

        MaterialRenderType targetMatType;
        switch (type)
        {
        case RenderType::Opaque:      targetMatType = MaterialRenderType::Opaque; break;
        case RenderType::Cutout:      targetMatType = MaterialRenderType::Cutout; break;
        case RenderType::Transparent: targetMatType = MaterialRenderType::Transparent; break;
        default: return false;
        }

        for (const auto& mat : m_materialData->GetMaterials())
        {
            if (mat.renderType == targetMatType) return true;
        }

        return false;
    }

    void SkeletalMeshRenderer::Draw(RenderType type) const
    {
        if (!m_meshData)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride(); // 자동 (Common or BoneWeight)

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        // Sampler
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Bone),
            1, m_boneConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_boneConstantBuffer->GetRawBuffer(), 0, nullptr, &m_boneTransformData, 0, 0);

        // CbObject 준비
        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();
        cbObject.worldInverseTranspose = GetTransform()->GetWorld().Invert();
        cbObject.boneIndex = -1; // 기본값

        // Material 기본값 설정
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

        // --- RenderType Switch ---
        switch (type)
        {
        case RenderType::Shadow:
            deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);
            break;
        case RenderType::Opaque:
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetShader(m_opaquePS->GetRawShader(), nullptr, 0);
            break;
        case RenderType::Cutout:
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetShader(m_cutoutPS->GetRawShader(), nullptr, 0);
            break;
        case RenderType::Transparent:
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
            deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);
            break;
        }

        // --- Sections Loop ---
        const auto& meshSections = m_meshData->GetMeshSections();
        const auto& materials = m_materialData->GetMaterials();

        for (const auto& section : meshSections)
        {
            // 필터링
            MaterialRenderType matType = materials[section.materialIndex].renderType;
            
            if (type == RenderType::Shadow)
            {
                if (matType == MaterialRenderType::Transparent) continue; // 투명은 그림자 X

                // Shadow PS 설정 (섹션별)
                if (matType == MaterialRenderType::Cutout)
                {
                    deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);
                    const auto textureSRVs = m_textures[section.materialIndex].AsRawArray();
                    if (!textureSRVs.empty()) 
                         deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &textureSRVs[0]); // BaseColor만
                }
                else // Opaque
                {
                    deviceContext->PSSetShader(nullptr, nullptr, 0);
                }
            }
            else // 일반 패스 (Opaque, Cutout, Trans)
            {
                MaterialRenderType targetType;
                if (type == RenderType::Opaque) targetType = MaterialRenderType::Opaque;
                else if (type == RenderType::Cutout) targetType = MaterialRenderType::Cutout;
                else targetType = MaterialRenderType::Transparent;

                if (matType != targetType) continue;

                // 텍스처 바인딩
                const auto textureSRVs = m_textures[section.materialIndex].AsRawArray();
                deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), static_cast<UINT>(textureSRVs.size()), textureSRVs.data());
            }

            // Rigid Mesh라면 boneIndex 업데이트
            if (m_meshData->IsRigid())
            {
                cbObject.boneIndex = section.boneIndex;
            }
            else
            {
                cbObject.boneIndex = -1; 
            }

            // Object Buffer 업데이트 (World + BoneIndex)
            deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
            deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

            // Draw
            deviceContext->DrawIndexed(section.indexCount, section.indexOffset, section.vertexOffset);
        }
    }

    DirectX::BoundingBox SkeletalMeshRenderer::GetBounds() const
    {
        return DirectX::BoundingBox(); // TODO: 계산 필요
    }

    void SkeletalMeshRenderer::DrawMask() const
    {
        if (!m_meshData)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride(); // 자동 (Common or BoneWeight)

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        // Sampler
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Bone),
            1, m_boneConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_boneConstantBuffer->GetRawBuffer(), 0, nullptr, &m_boneTransformData, 0, 0);

        // CbObject 준비
        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();
        cbObject.boneIndex = -1; // 기본값

        deviceContext->VSSetShader(m_simpleVS->GetRawShader(), nullptr, 0);
        deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

        const auto& meshSections = m_meshData->GetMeshSections();
        const auto& materials = m_materialData->GetMaterials();

        for (const auto& section : meshSections)
        {
            // 필터링
            MaterialRenderType matType = materials[section.materialIndex].renderType;

            const auto textureSRVs = m_textures[section.materialIndex].AsRawArray();
            if (!textureSRVs.empty())
            {
                deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &textureSRVs[0]); // BaseColor만
            }

            // Rigid Mesh라면 boneIndex 업데이트
            if (m_meshData->IsRigid())
            {
                cbObject.boneIndex = section.boneIndex;
            }
            else
            {
                cbObject.boneIndex = -1;
            }

            // Object Buffer 업데이트 (World + BoneIndex)
            deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);

            // Draw
            deviceContext->DrawIndexed(section.indexCount, section.indexOffset, section.vertexOffset);
        }
    }

    void SkeletalMeshRenderer::DrawPickingID() const
    {
        if (!m_meshData)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        static const UINT s_vertexBufferOffset = 0;
        const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride(); // 자동 (Common or BoneWeight)

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());

        // Sampler
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Bone),
            1, m_boneConstantBuffer->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_boneConstantBuffer->GetRawBuffer(), 0, nullptr, &m_boneTransformData, 0, 0);

        // CbObject 준비
        CbObject cbObject{};
        cbObject.world = GetTransform()->GetWorld().Transpose();
        cbObject.boneIndex = -1; // 기본값

        deviceContext->VSSetShader(m_simpleVS->GetRawShader(), nullptr, 0);
        deviceContext->PSSetShader(m_pickingPS->GetRawShader(), nullptr, 0);

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

        const auto& meshSections = m_meshData->GetMeshSections();
        const auto& materials = m_materialData->GetMaterials();

        for (const auto& section : meshSections)
        {
            // 필터링
            MaterialRenderType matType = materials[section.materialIndex].renderType;

            const auto textureSRVs = m_textures[section.materialIndex].AsRawArray();
            if (!textureSRVs.empty())
            {
                deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &textureSRVs[0]); // BaseColor만
            }

            // Rigid Mesh라면 boneIndex 업데이트
            if (m_meshData->IsRigid())
            {
                cbObject.boneIndex = section.boneIndex;
            }
            else
            {
                cbObject.boneIndex = -1;
            }

            // Object Buffer 업데이트 (World + BoneIndex)
            deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);

            // Draw
            deviceContext->DrawIndexed(section.indexCount, section.indexOffset, section.vertexOffset);
        }
    }

    void SkeletalMeshRenderer::Save(json& j) const
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

    void SkeletalMeshRenderer::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j, "MeshFilePath", m_meshFilePath);
        JsonGet(j, "VSFilePath", m_vsFilePath);
        JsonGet(j, "OpaquePSFilePath", m_opaquePSFilePath);
        JsonGet(j, "CutoutPSFilePath", m_cutoutPSFilePath);
        JsonGet(j, "TransparentPSFilePath", m_transparentPSFilePath);
        JsonGet(j, "MaterialBaseColor", m_materialBaseColor);
        JsonGet(j, "MaterialEmissive", m_materialEmissive);
        JsonGet(j, "MaterialRoughness", m_materialRoughness);
        JsonGet(j, "MaterialMetalness", m_materialMetalness);
        JsonGet(j, "MaterialAmbientOcclusion", m_materialAmbientOcclusion);
        JsonGet(j, "OverrideMaterial", m_overrideMaterial);

        Refresh();
    }

    void SkeletalMeshRenderer::OnGui()
    {
        // 1. Mesh Selector
        ImGui::Text("Mesh: %s", m_meshFilePath.c_str());
        std::string selectedMesh;
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
        static const std::string shaderPath = "Resource/Shader/Pixel";
        ImGui::Text("Shaders:");
        std::string selectedShader;
        if (DrawFileSelector("Opaque PS", shaderPath, ".hlsl", selectedShader))
        {
            SetOpaquePixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_opaquePSFilePath).filename().string().c_str());

        if (DrawFileSelector("Cutout PS", shaderPath, ".hlsl", selectedShader))
        {
            SetCutoutPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_cutoutPSFilePath).filename().string().c_str());
        if (DrawFileSelector("Trans PS", shaderPath, ".hlsl", selectedShader))
        {
            SetTransparentPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::filesystem::path(m_transparentPSFilePath).filename().string().c_str());
        // 3. Material Info
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
}