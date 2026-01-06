#include "EnginePCH.h"
#include "SpriteRenderer.h"

#include <filesystem>

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/SimpleMeshData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
    namespace
    {
        bool DrawFileSelector(const char* label, const std::string& basePath, const std::string& extension, std::string& outSelectedPath)
        {
            namespace fs = std::filesystem;
            bool result = false;
            if (ImGui::Button(label))
            {
                ImGui::OpenPopup(label);
            }
            if (ImGui::BeginPopupModal(label, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Separator();
                ImGui::BeginChild("FileList", ImVec2(400, 300), true);
                if (fs::exists(basePath))
                {
                    for (const auto& entry : fs::recursive_directory_iterator(basePath))
                    {
                        // 이미지 확장자 체크 로직 강화 가능
                        if (entry.is_regular_file())
                        {
                            std::string ext = entry.path().extension().string();
                            // 대소문자 무시 비교 필요할 수 있음
                            if (ext == extension || (extension == ".png" && (ext == ".jpg" || ext == ".dds" || ext == ".tga")))
                            {
                                std::string fullPath = entry.path().generic_string();
                                if (ImGui::Selectable(fullPath.c_str()))
                                {
                                    outSelectedPath = fullPath;
                                    result = true;
                                    ImGui::CloseCurrentPopup();
                                }
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

    SpriteRenderer::~SpriteRenderer()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);
    }

    void SpriteRenderer::Initialize()
    {
        m_vsFilePath = "Resource/Shader/Vertex/Quad_VS.hlsl";
        m_opaquePSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_PS.hlsl";
        m_cutoutPSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_Cutout_PS.hlsl";
        m_transparentPSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_Transparent_PS.hlsl";

        m_simpleMeshData = AssetManager::Get().GetOrCreateSimpleMeshData("Resource/Model/Quad.fbx");

        m_texture = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);

        m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<PositionTexCoordVertex>("Resource/Model/Quad.fbx", m_simpleMeshData->GetVertices());
        m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer("Resource/Model/Quad.fbx", m_simpleMeshData->GetIndices());

        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
        m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Quad_VS.hlsl");

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
        m_shadowCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Sprite_Unlit_Cutout_PS.hlsl");

        m_inputLayout = m_vs->GetOrCreateInputLayout<PositionTexCoordVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

        m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));

        SystemManager::Get().GetRenderSystem().Register(this);
    }

    void SpriteRenderer::SetTexture(const std::string& textureFilePath)
    {
        m_texture = ResourceManager::Get().GetOrCreateTexture(textureFilePath);
    }

    void SpriteRenderer::SetVertexShader(const std::string& shaderFilePath)
    {
        m_vsFilePath = shaderFilePath;
        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
    }

    void SpriteRenderer::SetOpaquePixelShader(const std::string& shaderFilePath)
    {
        m_opaquePSFilePath = shaderFilePath;
        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
    }

    void SpriteRenderer::SetCutoutPixelShader(const std::string& shaderFilePath)
    {
        m_cutoutPSFilePath = shaderFilePath;
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
    }

    void SpriteRenderer::SetTransparentPixelShader(const std::string& shaderFilePath)
    {
        m_transparentPSFilePath = shaderFilePath;
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
    }

    void SpriteRenderer::SetCastShadow(bool castShadow)
    {
        m_castShadow = castShadow;
    }

    void SpriteRenderer::OnGui()
    {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Sprite Renderer");
        ImGui::Separator();
        // 1. Texture Selector
        ImGui::Text("Texture: %s", std::filesystem::path(m_textureFilePath).filename().string().c_str());
        std::string selectedTex;
        // 확장자는 대표적으로 .png를 넣었지만 내부 로직에서 여러 개 체크하도록 수정 가능
        if (DrawFileSelector("Select Texture", "Resource/Texture", ".png", selectedTex))
        {
            SetTexture(selectedTex);
        }
        ImGui::Spacing();
        // 2. Settings (RenderType, Shadow, etc.)
        // RenderType (Enum to Checkbox or Combo)
        const char* renderTypes[] = { "Opaque", "Cutout", "Transparent" };
        int currentType = 0;
        if (m_renderType == MaterialRenderType::Cutout) currentType = 1;
        else if (m_renderType == MaterialRenderType::Transparent) currentType = 2;
        if (ImGui::Combo("Render Type", &currentType, renderTypes, IM_ARRAYSIZE(renderTypes)))
        {
            if (currentType == 0)
            {
                m_renderType = MaterialRenderType::Opaque;
            }
            else if (currentType == 1)
            {
                m_renderType = MaterialRenderType::Cutout;
            }
            else if (currentType == 2)
            {
                m_renderType = MaterialRenderType::Transparent;
            }

            ReplaceRenderSystem();
        }
        bool castShadow = m_castShadow;
        if (ImGui::Checkbox("Cast Shadow", &castShadow))
        {
            SetCastShadow(castShadow);
        }
        // PPU or Filter Mode (Example)
        // bool usePoint = ...;
        // if (ImGui::Checkbox("Point Filter", &usePoint)) { SetFilter(usePoint ? Point : Linear); }

        ImGui::Spacing();
        ImGui::Separator();
        // 3. Shader Selectors
        static const std::string shaderPath = "Resource/Shader/Pixel";
        static const std::string vertexShaderPath = "Resource/Shader/Vertex";
        ImGui::Text("Shaders:");
        std::string selectedShader;
        // Opaque
        if (DrawFileSelector("VS", vertexShaderPath, ".hlsl", selectedShader))
        {
            SetVertexShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_vsFilePath).filename().string().c_str());
        // Opaque PS
        if (DrawFileSelector("Opaque PS", shaderPath, ".hlsl", selectedShader))
        {
            SetOpaquePixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_opaquePSFilePath).filename().string().c_str());
        // Cutout PS
        if (DrawFileSelector("Cutout PS", shaderPath, ".hlsl", selectedShader))
        {
            SetCutoutPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_cutoutPSFilePath).filename().string().c_str());
        // Transparent PS
        if (DrawFileSelector("Trans PS", shaderPath, ".hlsl", selectedShader))
        {
            SetTransparentPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_transparentPSFilePath).filename().string().c_str());
    }

    void SpriteRenderer::Save(json& j) const
    {
        j["Type"] = GetType();
        j["TextureFilePath"] = m_textureFilePath;
        j["VSFilePath"] = m_vsFilePath;
        j["OpaquePSFilePath"] = m_opaquePSFilePath;
        j["CutoutPSFilePath"] = m_cutoutPSFilePath;
        j["TransparentPSFilePath"] = m_transparentPSFilePath;
    }

    void SpriteRenderer::Load(const json& j)
    {
        JsonGet(j, "MeshFilePath", m_textureFilePath);
        JsonGet(j, "VSFilePath", m_vsFilePath);
        JsonGet(j, "OpaquePSFilePath", m_opaquePSFilePath);
        JsonGet(j, "CutoutPSFilePath", m_cutoutPSFilePath);
        JsonGet(j, "TransparentPSFilePath", m_transparentPSFilePath);

        Refresh();
    }

    std::string SpriteRenderer::GetType() const
    {
        return "SpriteRenderer";
    }

    bool SpriteRenderer::HasRenderType(RenderType type) const
    {
        if (type == RenderType::Shadow)
        {
            return m_castShadow;
        }

        switch (m_renderType)
        {
        case MaterialRenderType::Opaque:
            if (type == RenderType::Opaque)
            {
                return true;
            }
            break;

        case MaterialRenderType::Cutout:
            if (type == RenderType::Cutout)
            {
                return true;
            }
            break;

        case MaterialRenderType::Transparent:
            if (type == RenderType::Transparent)
            {
                return true;
            }
            break;
        }

        return false;
    }

    void SpriteRenderer::Draw(RenderType type) const
    {
        // 텍스처가 없으면 그릴 수 없음
        if (!m_texture)
        {
            return;
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        // 1. 공통 State 설정 (IA)
        static const UINT stride = m_vertexBuffer->GetBufferStride();
        static const UINT offset = 0;

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &stride, &offset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R16_UINT, 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout()); // PositionTexCoordVertex 레이아웃

        // 2. World Matrix 계산 (PPU 적용)
        // 100 픽셀 = 1 유닛 (프로젝트 정책에 따라 상수화 추천)
        const float ppu = 100.0f;
        float width = m_width;//m_texture->GetWidth();
        float height = m_height;// m_texture->GetHeight();

        // 이미지 원본 비율에 맞춰 스케일 적용
        Vector3 imageScale(width / ppu, height / ppu, 1.0f);

        // (Local Quad 1x1) * (ImageRatio) * (Transform)
        Matrix scaleMatrix = Matrix::CreateScale(imageScale);
        Matrix finalWorld = scaleMatrix * GetTransform()->GetWorld();

        CbObject cbObject{};
        cbObject.world = finalWorld.Transpose();
        cbObject.worldInverseTranspose = finalWorld.Invert(); // Normal 계산용
        cbObject.boneIndex = -1;

        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

        // Sampler (Point or Linear 확인하여 바인딩)
        // 여기선 m_samplerState가 이미 Initialize 혹은 OnGui에서 설정되었다고 가정
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        // 3. RenderType 분기
        // -----------------------------------------------------------------------
        // [Shadow Pass]
        // -----------------------------------------------------------------------
        if (type == RenderType::Shadow)
        {
            if (!m_castShadow) return; // 그림자 끄기 옵션
            if (m_renderType == MaterialRenderType::Transparent) return; // 투명 객체는 보통 그림자 X

            // Shadow Vertex Shader
            // (주의: Static Mesh용 Shadow VS 사용. Skinned 아님)
            deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);

            // Cutout(Alpha Test) 일 경우 Pixel Shader 필요
            if (m_renderType == MaterialRenderType::Cutout)
            {
                deviceContext->PSSetShader(m_shadowCutoutPS->GetRawShader(), nullptr, 0);
                ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
                deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);
            }
            else // Opaque
            {
                deviceContext->PSSetShader(nullptr, nullptr, 0);
            }

            // Quad Index Draw (6 indices)
            deviceContext->DrawIndexed(6, 0, 0);
        }
        // -----------------------------------------------------------------------
        // [Main Pass] (Opaque, Cutout, Transparent)
        // -----------------------------------------------------------------------
        else
        {
            // 현재 렌더링 패스와 내 머티리얼 타입이 일치하는지 확인
            MaterialRenderType targetType;
            if (type == RenderType::Opaque) targetType = MaterialRenderType::Opaque;
            else if (type == RenderType::Cutout) targetType = MaterialRenderType::Cutout;
            else targetType = MaterialRenderType::Transparent;

            if (m_renderType != targetType) return;


            // -- Vertex Shader --
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);


            // -- Material Data --
            CbMaterial cbMaterial{};
            cbMaterial.materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // Tint Color가 있다면 여기에 적용
            cbMaterial.materialEmissive = Vector3(0, 0, 0);
            cbMaterial.materialRoughness = 1.0f; // 스프라이트는 거칠게 (반사 적게)
            cbMaterial.materialMetalness = 0.0f;

            deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());


            // -- Pixel Shader Selection --
            if (m_renderType == MaterialRenderType::Opaque)
                deviceContext->PSSetShader(m_opaquePS->GetRawShader(), nullptr, 0);
            else if (m_renderType == MaterialRenderType::Cutout)
                deviceContext->PSSetShader(m_cutoutPS->GetRawShader(), nullptr, 0);
            else if (m_renderType == MaterialRenderType::Transparent)
                deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);


            // -- Texture Binding --
            ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
            deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);


            // -- Draw --
            deviceContext->DrawIndexed(6, 0, 0);
        }
    }

    DirectX::BoundingBox SpriteRenderer::GetBounds() const
    {
        return DirectX::BoundingBox();
    }

    void SpriteRenderer::Refresh()
    {
        m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);

        m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
        m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
        m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
    }

    void SpriteRenderer::ReplaceRenderSystem()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);
        SystemManager::Get().GetRenderSystem().Register(this);
    }
}