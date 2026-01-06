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

namespace engine
{
     namespace
    {
        // [헬퍼] 파일 선택 모달 창 리턴값: 파일 선택 시 true
        bool DrawFileSelector(const char* label, const std::string& basePath, const std::string& extension, std::string& outSelectedPath)
        {
            namespace fs = std::filesystem;
            bool result = false;
            // 1. 버튼 그리기
            if (ImGui::Button(label))
            {
                ImGui::OpenPopup(label);
            }
            // 2. 모달 창
            // (항상 화면 중앙에 띄우려면 ImGuiWindowFlags_NoMove 추가)
            if (ImGui::BeginPopupModal(label, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                // 닫기 버튼 (우측 상단 X 대신 직접 구현)
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();
                // 리스트 박스 크기 제한
                ImGui::BeginChild("FileList", ImVec2(400, 300), true);
                if (fs::exists(basePath))
                {
                    // 재귀적으로 폴더 순회
                    for (const auto& entry : fs::recursive_directory_iterator(basePath))
                    {
                        if (entry.is_regular_file() && entry.path().extension() == extension)
                        {
                            // 보여줄 이름 (상대 경로로 깔끔하게)
                            // generic_string(): 윈도우 역슬래시(\)를 슬래시(/)로 통일
                            std::string fullPath = entry.path().generic_string();
                            std::string fileName = entry.path().filename().string();

                            // 목록 선택
                            if (ImGui::Selectable(fullPath.c_str())) // 전체 경로 보여주기 (동명이인 방지)
                                //if (ImGui::Selectable(fileName.c_str())) // 파일명만 보기
                            {
                                outSelectedPath = fullPath;
                                result = true;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Path not found: %s", basePath.c_str());
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
            return result;
        }
    }

    SkeletalMeshRenderer::SkeletalMeshRenderer() = default;

    SkeletalMeshRenderer::~SkeletalMeshRenderer()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);
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

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
        m_shadowCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Shadow_Cutout_PS.hlsl");
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

                m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(m_meshFilePath, m_meshData->GetVertices());
                
                m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
            }
            else
            {
                // Skinned Mesh (BoneWeightVertex)
                m_vsFilePath = "Resource/Shader/Vertex/Skinned_VS.hlsl";
                m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
                m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Shadow_Skinned_VS.hlsl");

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

        // Bone Buffer 바인딩 (Slot 3) - Rigid도 참조할 수 있음
        deviceContext->VSSetConstantBuffers(3, 1, m_boneConstantBuffer->GetBuffer().GetAddressOf());

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
            cbMaterial.materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
            cbMaterial.materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
            cbMaterial.materialRoughness = 0.0f;
            cbMaterial.materialMetalness = 0.0f;
            cbMaterial.materialAmbientOcclusion = 1.0f;
            cbMaterial.overrideMaterial = 0;
            deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
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
                    deviceContext->PSSetShader(m_shadowCutoutPS->GetRawShader(), nullptr, 0);
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

    void SkeletalMeshRenderer::Save(json& j) const
    {
        j["Type"] = GetType();
        j["MeshFilePath"] = m_meshFilePath;
        j["VSFilePath"] = m_vsFilePath;
        j["OpaquePSFilePath"] = m_opaquePSFilePath;
        j["CutoutPSFilePath"] = m_cutoutPSFilePath;
        j["TransparentPSFilePath"] = m_transparentPSFilePath;
    }

    void SkeletalMeshRenderer::Load(const json& j)
    {
        JsonGet(j, "MeshFilePath", m_meshFilePath);
        JsonGet(j, "VSFilePath", m_vsFilePath);
        JsonGet(j, "OpaquePSFilePath", m_opaquePSFilePath);
        JsonGet(j, "CutoutPSFilePath", m_cutoutPSFilePath);
        JsonGet(j, "TransparentPSFilePath", m_transparentPSFilePath);

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