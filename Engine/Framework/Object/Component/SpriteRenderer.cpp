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
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/SimpleMeshData.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/Camera.h"
#include "Common/Utility/StaticMemoryPool.h"

void to_json(nlohmann::ordered_json& j, engine::MaterialRenderType type)
{
    j = nlohmann::ordered_json{ "RenderType", static_cast<int>(type) };
}

void from_json(const nlohmann::ordered_json& j, engine::MaterialRenderType& type)
{
    type = static_cast<engine::MaterialRenderType>(j.at("RenderType"));
}

void to_json(nlohmann::ordered_json& j, engine::CullMode mode)
{
    j = nlohmann::ordered_json{ "CullMode", static_cast<int>(mode) };
}

void from_json(const nlohmann::ordered_json& j, engine::CullMode& mode)
{
    mode = static_cast<engine::CullMode>(j.at("CullMode"));
}

void to_json(nlohmann::ordered_json& j, engine::BillboardType type)
{
    j = nlohmann::ordered_json{ "BillboardType", static_cast<int>(type) };
}

void from_json(const nlohmann::ordered_json& j, engine::BillboardType& type)
{
    type = static_cast<engine::BillboardType>(j.at("BillboardType"));
}

namespace engine
{
    namespace
    {
        StaticMemoryPool<SpriteRenderer, 1024> g_spriteRendererPool;
    }

    SpriteRenderer::~SpriteRenderer()
    {
        SystemManager::Get().GetRenderSystem().Unregister(this);
    }

    void* SpriteRenderer::operator new(size_t size)
    {
        return g_spriteRendererPool.Allocate(size);
    }

    void SpriteRenderer::operator delete(void* ptr)
    {
        g_spriteRendererPool.Deallocate(ptr);
    }

    void SpriteRenderer::Initialize()
    {
        if (m_texture == nullptr)
        {
            m_texture = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
            m_textureFilePath = "None";
        }

        if (!m_isLoaded)
        {
            m_vsFilePath = "Resource/Shader/Vertex/Quad_VS.hlsl";
            m_opaquePSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_PS.hlsl";
            m_cutoutPSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_Cutout_PS.hlsl";
            m_transparentPSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_Transparent_PS.hlsl";

            m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
            m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
            m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
            m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
        }

        m_vertexBuffer = ResourceManager::Get().GetGeometryVertexBuffer("DefaultQuad");
        m_indexBuffer = ResourceManager::Get().GetGeometryIndexBuffer("DefaultQuad");

        m_shadowVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Quad_VS.hlsl");

        m_maskCutoutPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Mask_Cutout_PS.hlsl");
        m_pickingPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Picking_PS.hlsl");

        m_inputLayout = m_vs->GetOrCreateInputLayout<PositionTexCoordVertex>();
        m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

        m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
        m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));
        m_spriteConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Sprite", sizeof(CbSprite));

        m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidNone);

        SystemManager::Get().GetRenderSystem().Register(this);
    }

    void SpriteRenderer::SetTexture(const std::string& textureFilePath)
    {
        if (textureFilePath.empty() || textureFilePath == "None")
        {
            return;
        }

        m_textureFilePath = textureFilePath;

        m_texture = ResourceManager::Get().GetOrCreateTexture(textureFilePath);
        m_width = m_texture->GetWidth();
        m_height = m_texture->GetHeight();
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

    void SpriteRenderer::SetCullMode(CullMode cullMode)
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

    void SpriteRenderer::SetSpriteInfo(const Vector2& offset, const Vector2& scale, const Vector2 pivot)
    {
        m_uvOffset = offset;
        m_uvScale = scale;
        m_pivot = pivot;
    }

    void SpriteRenderer::SetBillboardType(BillboardType type)
    {
        m_billboardType = type;
    }

    void SpriteRenderer::OnGui()
    {
        ImGui::Text("Texture: %s", std::filesystem::path(m_textureFilePath).filename().string().c_str());
        std::string selectedTex;
        
        static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga" };
        static std::string hlslExtension{ ".hlsl" };

        if (DrawFileSelector("Select Texture", "Resource/Texture", texExtensions, selectedTex))
        {
            SetTexture(selectedTex);
        }
        ImGui::Spacing();
        // 2. Settings (RenderType, Shadow, etc.)
        // RenderType (Enum to Checkbox or Combo)
        static const char* renderTypes[] = { "Opaque", "Cutout", "Transparent" };
        int currentType = static_cast<int>(m_renderType);
        if (ImGui::Combo("Render Type", &currentType, renderTypes, IM_ARRAYSIZE(renderTypes)))
        {
            m_renderType = static_cast<MaterialRenderType>(currentType);

            ReplaceRenderSystem();
        }
        bool castShadow = m_castShadow;
        if (ImGui::Checkbox("Cast Shadow", &castShadow))
        {
            SetCastShadow(castShadow);
        }

        ImGui::ColorEdit4("Color", &m_color.x);

        static const char* cullModes[] = { "None", "Back", "Front" };
        int currentMode = static_cast<int>(m_cullMode);
        if (ImGui::Combo("Cull mode", &currentMode, cullModes, IM_ARRAYSIZE(cullModes)))
        {
            m_cullMode = static_cast<CullMode>(currentMode);

            SetCullMode(m_cullMode);
        }

        static const char* billboardTypes[] = { "None", "Spherical", "Cylindrical", "ViewPlaneAligned", "ViewPlaneVertical" };
        int currentBillboard = static_cast<int>(m_billboardType);
        if (ImGui::Combo("Billboard", &currentBillboard, billboardTypes, IM_ARRAYSIZE(billboardTypes)))
        {
            m_billboardType = static_cast<BillboardType>(currentBillboard);
        }

        ImGui::Spacing();
        ImGui::Separator();
        // 3. Shader Selectors
        static const std::string shaderPath = "Resource/Shader/Pixel";
        static const std::string vertexShaderPath = "Resource/Shader/Vertex";
        ImGui::Text("Shaders:");
        std::string selectedShader;
        // Opaque
        if (DrawFileSelector("VS", vertexShaderPath, hlslExtension, selectedShader))
        {
            SetVertexShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_vsFilePath).filename().string().c_str());
        // Opaque PS
        if (DrawFileSelector("Opaque PS", shaderPath, hlslExtension, selectedShader))
        {
            SetOpaquePixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_opaquePSFilePath).filename().string().c_str());
        // Cutout PS
        if (DrawFileSelector("Cutout PS", shaderPath, hlslExtension, selectedShader))
        {
            SetCutoutPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_cutoutPSFilePath).filename().string().c_str());
        // Transparent PS
        if (DrawFileSelector("Trans PS", shaderPath, hlslExtension, selectedShader))
        {
            SetTransparentPixelShader(selectedShader);
        }
        ImGui::SameLine();
        ImGui::Text(std::filesystem::path(m_transparentPSFilePath).filename().string().c_str());
    }

    void SpriteRenderer::Save(json& j) const
    {
        Object::Save(j);

        j["TextureFilePath"] = m_textureFilePath ;
        j["VSFilePath"] = m_vsFilePath;
        j["OpaquePSFilePath"] = m_opaquePSFilePath;
        j["CutoutPSFilePath"] = m_cutoutPSFilePath;
        j["TransparentPSFilePath"] = m_transparentPSFilePath;
        j["RenderType"] = m_renderType;
        j["BillboardType"] = m_billboardType;
        j["Color"] = m_color;
        j["CullMode"] = m_cullMode;
    }

    void SpriteRenderer::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j, "TextureFilePath", m_textureFilePath);
        JsonGet(j, "VSFilePath", m_vsFilePath);
        JsonGet(j, "OpaquePSFilePath", m_opaquePSFilePath);
        JsonGet(j, "CutoutPSFilePath", m_cutoutPSFilePath);
        JsonGet(j, "TransparentPSFilePath", m_transparentPSFilePath);
        JsonGet(j, "RenderType", m_renderType);
        JsonGet(j, "BillboardType", m_billboardType);
        JsonGet(j, "Color", m_color);
        JsonGet(j, "CullMode", m_cullMode);

        m_isLoaded = true;

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

        if (type == RenderType::Shadow)
        {
            if (!m_castShadow || m_renderType == MaterialRenderType::Transparent)
            {
                return;
            }
        }

        const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();

        // 1. 공통 State 설정 (IA)
        static const UINT stride = m_vertexBuffer->GetBufferStride();
        static const UINT offset = 0;

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &stride, &offset);
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), m_indexBuffer->GetIndexFormat(), 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout()); // PositionTexCoordVertex 레이아웃

        // 100 픽셀 = 1 유닛 (프로젝트 정책에 따라 상수화 추천)
        constexpr float ppu = 100.0f;

        // 이미지 원본 비율에 맞춰 스케일 적용
        const Vector3 imageScale(m_width / ppu * m_uvScale.x, m_height / ppu * m_uvScale.y, 1.0f);

        // (Local Quad 1x1) * (ImageRatio) * (Transform)
        const Matrix scaleMatrix = Matrix::CreateScale(imageScale);
        //const Matrix finalWorld = scaleMatrix * GetTransform()->GetWorld();

        Matrix finalWorld;

        if (m_billboardType == BillboardType::None)
        {
            // 기본: Transform의 World 행렬 그대로 사용
            finalWorld = scaleMatrix * GetTransform()->GetWorld();
        }
        else
        {
            // 빌보드 처리
            auto transform = GetTransform();
            Vector3 worldPos = transform->GetWorldPosition();
            Vector3 worldScale = transform->GetLocalScale(); // 부모 스케일 무시하고 로컬 스케일만 적용? 혹은 GetWorldScale() 필요
            // 카메라 정보 가져오기
            // SceneManager를 통해 현재 활성 씬의 카메라를 가져옵니다.
            auto scene = SceneManager::Get().GetScene();
            auto camera = scene ? scene->GetMainCamera() : nullptr;
            if (camera)
            {
                Vector3 cameraPos = camera->GetTransform()->GetWorldPosition();
                Vector3 forward = cameraPos - worldPos; // Sprite -> Camera 방향

                if (m_billboardType == BillboardType::Spherical)
                {
                    // 모든 축에서 바라봄
                    // forward 벡터를 정규화
                    forward.Normalize();

                    // Up 벡터는 카메라의 Up 벡터를 쓰거나, 월드 Up(0,1,0)과 forward를 이용해 계산
                    // 여기서는 간단하게 Matrix::CreateBillboard 사용
                    // 주의: CreateBillboard는 "Camera Position"과 "Object Position"을 인자로 받습니다.
                    // (DirectXMath 문서 확인 필요: CreateBillboard(object, cameraPos, cameraUp, cameraForward)

                    // Matrix::CreateBillboard:
                    // Builds a transformation matrix where the object is positioned at "objectPosition" 
                    // and faces "cameraPosition".

                    Vector3 cameraUp = camera->GetTransform()->GetUp();

                    Matrix billboard = Matrix::CreateBillboard(worldPos, cameraPos, cameraUp);

                    // Scale 적용 (이미지 비율 * 오브젝트 스케일)
                    // CreateBillboard는 회전/이동만 포함하므로 스케일링은 별도로 곱해줘야 함
                    // 순서: Scale -> Billboard

                    // 전체 스케일 (이미지 보정 * 오브젝트 스케일)
                    Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                    finalWorld = totalScale * billboard;
                }
                else if (m_billboardType == BillboardType::Cylindrical)
                {
                    // Y축 회전만 (수직 빌보드)
                    // Project forward vector to XZ plane
                    forward.y = 0.0f;
                    if (forward.LengthSquared() > 0.0001f) // 0 벡터 방지
                    {
                        forward.Normalize();

                        // Y축 회전 행렬 생성 (At, Eye, Up)
                        // LookAt은 View Matrix를 만드므로, World Matrix를 만들려면 CreateWorld나 LookTo 등을 써야 함.
                        // Matrix::CreateConstrainedBillboard 사용 가능
                        // Builds a transformation matrix where the object faces the camera but is constrained to rotate only around a specified axis.

                        Vector3 rotateAxis(0, 1, 0); // Y축

                        // CreateConstrainedBillboard(objPos, camPos, rotateAxis, camForward, objForward)
                        // 보통 objForward는 (0,0,1)

                        Matrix billboard = Matrix::CreateConstrainedBillboard(worldPos, cameraPos, rotateAxis);

                        Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                        finalWorld = totalScale * billboard;
                    }
                    else
                    {
                        // 바로 위/아래에 있어서 방향을 알 수 없는 경우 회전 안함
                        finalWorld = scaleMatrix * transform->GetWorld();
                    }
                }
                else if (m_billboardType == BillboardType::ViewPlaneAligned)
                {
                    // [NEW] 카메라 평면과 평행
                    // 카메라의 회전 행렬을 그대로 가져오면 됨
                    // (스프라이트의 Forward(-Z)가 카메라의 Forward(Z)와 반대가 되도록? 
                    //  보통 쿼드는 Z-가 앞인데 카메라는 Z+를 봄 (RH/LH 따름). 
                    //  단순하게는 카메라의 Rotation Matrix를 그대로 쓰면 됨)

                    // Camera World Matrix에서 Rotation 부분만 추출
                    Matrix camWorld = camera->GetWorld();

                    // Translation 제거
                    camWorld._41 = 0; camWorld._42 = 0; camWorld._43 = 0;

                    // Scale 제거 (카메라에 스케일이 있다면 문제될 수 있으니 정규화 필요)
                    Vector3 right = Vector3(camWorld._11, camWorld._12, camWorld._13);
                    Vector3 up = Vector3(camWorld._21, camWorld._22, camWorld._23);
                    Vector3 fwd = Vector3(camWorld._31, camWorld._32, camWorld._33);

                    right.Normalize();
                    up.Normalize();
                    fwd.Normalize();

                    Matrix billboard = Matrix::Identity;
                    billboard.Right(right);
                    billboard.Up(up);
                    billboard.Forward(fwd);

                    // 최종 행렬: Scale * Rotation(Camera) * Translation(Object)
                    Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                    Matrix translation = Matrix::CreateTranslation(worldPos);

                    finalWorld = totalScale * billboard * translation;
                }
                else if (m_billboardType == BillboardType::ViewPlaneVertical)
                {
                    // [NEW] View Plane Vertical
                    // 카메라의 Forward를 가져와서 Y축 성분을 제거하고 정규화
                    Vector3 cameraForward = camera->GetForward();
                    cameraForward.y = 0.0f;

                    if (cameraForward.LengthSquared() > 0.0001f)
                    {
                        cameraForward.Normalize();

                        // CreateWorld(pos, forward, up) - Y축은 (0,1,0) 고정
                        // Sprite의 Forward를 Camera의 ProjectOnXZ(Forward)와 일치시킴
                        Matrix bb = Matrix::CreateWorld(worldPos, -cameraForward, Vector3::UnitY);

                        Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                        finalWorld = totalScale * bb;
                    }
                    else
                    {
                        // 카메라가 완전히 위/아래를 볼 때는 회전하지 않음 (기존 transform 유지)
                        const Matrix scaleOriginal = Matrix::CreateScale(imageScale);
                        finalWorld = scaleOriginal * transform->GetWorld();
                    }
                }
            }
            else
            {
                // 카메라가 없으면 일반 렌더링
                finalWorld = scaleMatrix * transform->GetWorld();
            }
        }

        CbObject cbObject{};
        cbObject.world = finalWorld.Transpose();
        cbObject.worldInverseTranspose = finalWorld.Invert(); // Normal 계산용
        cbObject.boneIndex = -1;

        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

        CbSprite cbSprite{};
        cbSprite.uvOffset = m_uvOffset;
        cbSprite.uvScale = m_uvScale;
        cbSprite.pivot = m_pivot;

        deviceContext->UpdateSubresource(m_spriteConstantBuffer->GetRawBuffer(), 0, nullptr, &cbSprite, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Sprite), 1, m_spriteConstantBuffer->GetBuffer().GetAddressOf());

        // Sampler (Point or Linear 확인하여 바인딩)
        // 여기선 m_samplerState가 이미 Initialize 혹은 OnGui에서 설정되었다고 가정
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->RSSetState(m_rasterizerState->GetRawRasterizerState());

        if (type == RenderType::Shadow)
        {
            deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);

            if (m_renderType == MaterialRenderType::Cutout)
            {
                deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);
                ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
                deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);
            }
            else
            {
                deviceContext->PSSetShader(nullptr, nullptr, 0);
            }

            deviceContext->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);
        }
        else
        {
            switch (type)
            {
            case RenderType::Opaque:
                deviceContext->PSSetShader(m_opaquePS->GetRawShader(), nullptr, 0);
                break;

            case RenderType::Cutout:
                deviceContext->PSSetShader(m_cutoutPS->GetRawShader(), nullptr, 0);
                break;

            case RenderType::Transparent:
                deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);
                break;
            }
            
            deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);

            CbMaterial cbMaterial{};
            cbMaterial.materialBaseColor = m_color;
            cbMaterial.materialEmissive = Vector3(0.0f, 0.0f, 0.0f);
            cbMaterial.materialRoughness = 1.0f;
            cbMaterial.materialMetalness = 0.0f;

            deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);
            deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
            
            ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
            deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);

            deviceContext->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);
        }
    }

    DirectX::BoundingBox SpriteRenderer::GetBounds() const
    {
        return DirectX::BoundingBox();
    }

    void SpriteRenderer::DrawMask() const
    {
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
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), m_indexBuffer->GetIndexFormat(), 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout()); // PositionTexCoordVertex 레이아웃

        // 100 픽셀 = 1 유닛 (프로젝트 정책에 따라 상수화 추천)
        constexpr float ppu = 100.0f;

        // 이미지 원본 비율에 맞춰 스케일 적용
        const Vector3 imageScale(m_width / ppu * m_uvScale.x, m_height / ppu * m_uvScale.y, 1.0f);

        // (Local Quad 1x1) * (ImageRatio) * (Transform)
        const Matrix scaleMatrix = Matrix::CreateScale(imageScale);
        //const Matrix finalWorld = scaleMatrix * GetTransform()->GetWorld();

        Matrix finalWorld;

        if (m_billboardType == BillboardType::None)
        {
            // 기본: Transform의 World 행렬 그대로 사용
            finalWorld = scaleMatrix * GetTransform()->GetWorld();
        }
        else
        {
            // 빌보드 처리
            auto transform = GetTransform();
            Vector3 worldPos = transform->GetWorldPosition();
            Vector3 worldScale = transform->GetLocalScale(); // 부모 스케일 무시하고 로컬 스케일만 적용? 혹은 GetWorldScale() 필요
            // 카메라 정보 가져오기
            // SceneManager를 통해 현재 활성 씬의 카메라를 가져옵니다.
            auto scene = SceneManager::Get().GetScene();
            auto camera = scene ? scene->GetMainCamera() : nullptr;
            if (camera)
            {
                Vector3 cameraPos = camera->GetTransform()->GetWorldPosition();
                Vector3 forward = cameraPos - worldPos; // Sprite -> Camera 방향

                if (m_billboardType == BillboardType::Spherical)
                {
                    // 모든 축에서 바라봄
                    // forward 벡터를 정규화
                    forward.Normalize();

                    // Up 벡터는 카메라의 Up 벡터를 쓰거나, 월드 Up(0,1,0)과 forward를 이용해 계산
                    // 여기서는 간단하게 Matrix::CreateBillboard 사용
                    // 주의: CreateBillboard는 "Camera Position"과 "Object Position"을 인자로 받습니다.
                    // (DirectXMath 문서 확인 필요: CreateBillboard(object, cameraPos, cameraUp, cameraForward)

                    // Matrix::CreateBillboard:
                    // Builds a transformation matrix where the object is positioned at "objectPosition" 
                    // and faces "cameraPosition".

                    Vector3 cameraUp = camera->GetTransform()->GetUp();

                    Matrix billboard = Matrix::CreateBillboard(worldPos, cameraPos, cameraUp);

                    // Scale 적용 (이미지 비율 * 오브젝트 스케일)
                    // CreateBillboard는 회전/이동만 포함하므로 스케일링은 별도로 곱해줘야 함
                    // 순서: Scale -> Billboard

                    // 전체 스케일 (이미지 보정 * 오브젝트 스케일)
                    Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                    finalWorld = totalScale * billboard;
                }
                else if (m_billboardType == BillboardType::Cylindrical)
                {
                    // Y축 회전만 (수직 빌보드)
                    // Project forward vector to XZ plane
                    forward.y = 0.0f;
                    if (forward.LengthSquared() > 0.0001f) // 0 벡터 방지
                    {
                        forward.Normalize();

                        // Y축 회전 행렬 생성 (At, Eye, Up)
                        // LookAt은 View Matrix를 만드므로, World Matrix를 만들려면 CreateWorld나 LookTo 등을 써야 함.
                        // Matrix::CreateConstrainedBillboard 사용 가능
                        // Builds a transformation matrix where the object faces the camera but is constrained to rotate only around a specified axis.

                        Vector3 rotateAxis(0, 1, 0); // Y축

                        // CreateConstrainedBillboard(objPos, camPos, rotateAxis, camForward, objForward)
                        // 보통 objForward는 (0,0,1)

                        Matrix billboard = Matrix::CreateConstrainedBillboard(worldPos, cameraPos, rotateAxis);

                        Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                        finalWorld = totalScale * billboard;
                    }
                    else
                    {
                        // 바로 위/아래에 있어서 방향을 알 수 없는 경우 회전 안함
                        finalWorld = scaleMatrix * transform->GetWorld();
                    }
                }
                else if (m_billboardType == BillboardType::ViewPlaneAligned)
                {
                    // [NEW] 카메라 평면과 평행
                    // 카메라의 회전 행렬을 그대로 가져오면 됨
                    // (스프라이트의 Forward(-Z)가 카메라의 Forward(Z)와 반대가 되도록? 
                    //  보통 쿼드는 Z-가 앞인데 카메라는 Z+를 봄 (RH/LH 따름). 
                    //  단순하게는 카메라의 Rotation Matrix를 그대로 쓰면 됨)

                    // Camera World Matrix에서 Rotation 부분만 추출
                    Matrix camWorld = camera->GetWorld();

                    // Translation 제거
                    camWorld._41 = 0; camWorld._42 = 0; camWorld._43 = 0;

                    // Scale 제거 (카메라에 스케일이 있다면 문제될 수 있으니 정규화 필요)
                    Vector3 right = Vector3(camWorld._11, camWorld._12, camWorld._13);
                    Vector3 up = Vector3(camWorld._21, camWorld._22, camWorld._23);
                    Vector3 fwd = Vector3(camWorld._31, camWorld._32, camWorld._33);

                    right.Normalize();
                    up.Normalize();
                    fwd.Normalize();

                    Matrix billboard = Matrix::Identity;
                    billboard.Right(right);
                    billboard.Up(up);
                    billboard.Forward(fwd);

                    // 최종 행렬: Scale * Rotation(Camera) * Translation(Object)
                    Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                    Matrix translation = Matrix::CreateTranslation(worldPos);

                    finalWorld = totalScale * billboard * translation;
                }
                else if (m_billboardType == BillboardType::ViewPlaneVertical)
                {
                    // [NEW] View Plane Vertical
                    // 카메라의 Forward를 가져와서 Y축 성분을 제거하고 정규화
                    Vector3 cameraForward = camera->GetForward();
                    cameraForward.y = 0.0f;

                    if (cameraForward.LengthSquared() > 0.0001f)
                    {
                        cameraForward.Normalize();

                        // CreateWorld(pos, forward, up) - Y축은 (0,1,0) 고정
                        // Sprite의 Forward를 Camera의 ProjectOnXZ(Forward)와 일치시킴
                        Matrix bb = Matrix::CreateWorld(worldPos, -cameraForward, Vector3::UnitY);

                        Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                        finalWorld = totalScale * bb;
                    }
                    else
                    {
                        // 카메라가 완전히 위/아래를 볼 때는 회전하지 않음 (기존 transform 유지)
                        const Matrix scaleOriginal = Matrix::CreateScale(imageScale);
                        finalWorld = scaleOriginal * transform->GetWorld();
                    }
                }
            }
            else
            {
                // 카메라가 없으면 일반 렌더링
                finalWorld = scaleMatrix * transform->GetWorld();
            }
        }

        CbObject cbObject{};
        cbObject.world = finalWorld.Transpose();
        cbObject.worldInverseTranspose = finalWorld.Invert(); // Normal 계산용
        cbObject.boneIndex = -1;

        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

        CbSprite cbSprite{};
        cbSprite.uvOffset = m_uvOffset;
        cbSprite.uvScale = m_uvScale;
        cbSprite.pivot = m_pivot;

        deviceContext->UpdateSubresource(m_spriteConstantBuffer->GetRawBuffer(), 0, nullptr, &cbSprite, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Sprite), 1, m_spriteConstantBuffer->GetBuffer().GetAddressOf());

        // Sampler (Point or Linear 확인하여 바인딩)
        // 여기선 m_samplerState가 이미 Initialize 혹은 OnGui에서 설정되었다고 가정
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->RSSetState(m_rasterizerState->GetRawRasterizerState());

        deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);
        deviceContext->PSSetShader(m_maskCutoutPS->GetRawShader(), nullptr, 0);
        ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
        deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);

        deviceContext->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);
    }

    void SpriteRenderer::DrawPickingID() const
    {
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
        deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), m_indexBuffer->GetIndexFormat(), 0);
        deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout()); // PositionTexCoordVertex 레이아웃

        // 100 픽셀 = 1 유닛 (프로젝트 정책에 따라 상수화 추천)
        constexpr float ppu = 100.0f;

        // 이미지 원본 비율에 맞춰 스케일 적용
        const Vector3 imageScale(m_width / ppu * m_uvScale.x, m_height / ppu * m_uvScale.y, 1.0f);

        // (Local Quad 1x1) * (ImageRatio) * (Transform)
        const Matrix scaleMatrix = Matrix::CreateScale(imageScale);
        //const Matrix finalWorld = scaleMatrix * GetTransform()->GetWorld();

        Matrix finalWorld;

        if (m_billboardType == BillboardType::None)
        {
            // 기본: Transform의 World 행렬 그대로 사용
            finalWorld = scaleMatrix * GetTransform()->GetWorld();
        }
        else
        {
            // 빌보드 처리
            auto transform = GetTransform();
            Vector3 worldPos = transform->GetWorldPosition();
            Vector3 worldScale = transform->GetLocalScale(); // 부모 스케일 무시하고 로컬 스케일만 적용? 혹은 GetWorldScale() 필요
            // 카메라 정보 가져오기
            // SceneManager를 통해 현재 활성 씬의 카메라를 가져옵니다.
            auto scene = SceneManager::Get().GetScene();
            auto camera = scene ? scene->GetMainCamera() : nullptr;
            if (camera)
            {
                Vector3 cameraPos = camera->GetTransform()->GetWorldPosition();
                Vector3 forward = cameraPos - worldPos; // Sprite -> Camera 방향

                if (m_billboardType == BillboardType::Spherical)
                {
                    // 모든 축에서 바라봄
                    // forward 벡터를 정규화
                    forward.Normalize();

                    // Up 벡터는 카메라의 Up 벡터를 쓰거나, 월드 Up(0,1,0)과 forward를 이용해 계산
                    // 여기서는 간단하게 Matrix::CreateBillboard 사용
                    // 주의: CreateBillboard는 "Camera Position"과 "Object Position"을 인자로 받습니다.
                    // (DirectXMath 문서 확인 필요: CreateBillboard(object, cameraPos, cameraUp, cameraForward)

                    // Matrix::CreateBillboard:
                    // Builds a transformation matrix where the object is positioned at "objectPosition" 
                    // and faces "cameraPosition".

                    Vector3 cameraUp = camera->GetTransform()->GetUp();

                    Matrix billboard = Matrix::CreateBillboard(worldPos, cameraPos, cameraUp);

                    // Scale 적용 (이미지 비율 * 오브젝트 스케일)
                    // CreateBillboard는 회전/이동만 포함하므로 스케일링은 별도로 곱해줘야 함
                    // 순서: Scale -> Billboard

                    // 전체 스케일 (이미지 보정 * 오브젝트 스케일)
                    Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                    finalWorld = totalScale * billboard;
                }
                else if (m_billboardType == BillboardType::Cylindrical)
                {
                    // Y축 회전만 (수직 빌보드)
                    // Project forward vector to XZ plane
                    forward.y = 0.0f;
                    if (forward.LengthSquared() > 0.0001f) // 0 벡터 방지
                    {
                        forward.Normalize();

                        // Y축 회전 행렬 생성 (At, Eye, Up)
                        // LookAt은 View Matrix를 만드므로, World Matrix를 만들려면 CreateWorld나 LookTo 등을 써야 함.
                        // Matrix::CreateConstrainedBillboard 사용 가능
                        // Builds a transformation matrix where the object faces the camera but is constrained to rotate only around a specified axis.

                        Vector3 rotateAxis(0, 1, 0); // Y축

                        // CreateConstrainedBillboard(objPos, camPos, rotateAxis, camForward, objForward)
                        // 보통 objForward는 (0,0,1)

                        Matrix billboard = Matrix::CreateConstrainedBillboard(worldPos, cameraPos, rotateAxis);

                        Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                        finalWorld = totalScale * billboard;
                    }
                    else
                    {
                        // 바로 위/아래에 있어서 방향을 알 수 없는 경우 회전 안함
                        finalWorld = scaleMatrix * transform->GetWorld();
                    }
                }
                else if (m_billboardType == BillboardType::ViewPlaneAligned)
                {
                    // [NEW] 카메라 평면과 평행
                    // 카메라의 회전 행렬을 그대로 가져오면 됨
                    // (스프라이트의 Forward(-Z)가 카메라의 Forward(Z)와 반대가 되도록? 
                    //  보통 쿼드는 Z-가 앞인데 카메라는 Z+를 봄 (RH/LH 따름). 
                    //  단순하게는 카메라의 Rotation Matrix를 그대로 쓰면 됨)

                    // Camera World Matrix에서 Rotation 부분만 추출
                    Matrix camWorld = camera->GetWorld();

                    // Translation 제거
                    camWorld._41 = 0; camWorld._42 = 0; camWorld._43 = 0;

                    // Scale 제거 (카메라에 스케일이 있다면 문제될 수 있으니 정규화 필요)
                    Vector3 right = Vector3(camWorld._11, camWorld._12, camWorld._13);
                    Vector3 up = Vector3(camWorld._21, camWorld._22, camWorld._23);
                    Vector3 fwd = Vector3(camWorld._31, camWorld._32, camWorld._33);

                    right.Normalize();
                    up.Normalize();
                    fwd.Normalize();

                    Matrix billboard = Matrix::Identity;
                    billboard.Right(right);
                    billboard.Up(up);
                    billboard.Forward(fwd);

                    // 최종 행렬: Scale * Rotation(Camera) * Translation(Object)
                    Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                    Matrix translation = Matrix::CreateTranslation(worldPos);

                    finalWorld = totalScale * billboard * translation;
                }
                else if (m_billboardType == BillboardType::ViewPlaneVertical)
                {
                    // [NEW] View Plane Vertical
                    // 카메라의 Forward를 가져와서 Y축 성분을 제거하고 정규화
                    Vector3 cameraForward = camera->GetForward();
                    cameraForward.y = 0.0f;

                    if (cameraForward.LengthSquared() > 0.0001f)
                    {
                        cameraForward.Normalize();

                        // CreateWorld(pos, forward, up) - Y축은 (0,1,0) 고정
                        // Sprite의 Forward를 Camera의 ProjectOnXZ(Forward)와 일치시킴
                        Matrix bb = Matrix::CreateWorld(worldPos, -cameraForward, Vector3::UnitY);

                        Matrix totalScale = Matrix::CreateScale(imageScale * worldScale);
                        finalWorld = totalScale * bb;
                    }
                    else
                    {
                        // 카메라가 완전히 위/아래를 볼 때는 회전하지 않음 (기존 transform 유지)
                        const Matrix scaleOriginal = Matrix::CreateScale(imageScale);
                        finalWorld = scaleOriginal * transform->GetWorld();
                    }
                }
            }
            else
            {
                // 카메라가 없으면 일반 렌더링
                finalWorld = scaleMatrix * transform->GetWorld();
            }
        }

        CbObject cbObject{};
        cbObject.world = finalWorld.Transpose();
        cbObject.worldInverseTranspose = finalWorld.Invert(); // Normal 계산용
        cbObject.boneIndex = -1;

        deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

        CbSprite cbSprite{};
        cbSprite.uvOffset = m_uvOffset;
        cbSprite.uvScale = m_uvScale;
        cbSprite.pivot = m_pivot;

        deviceContext->UpdateSubresource(m_spriteConstantBuffer->GetRawBuffer(), 0, nullptr, &cbSprite, 0, 0);
        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Sprite), 1, m_spriteConstantBuffer->GetBuffer().GetAddressOf());

        // Sampler (Point or Linear 확인하여 바인딩)
        // 여기선 m_samplerState가 이미 Initialize 혹은 OnGui에서 설정되었다고 가정
        deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

        deviceContext->RSSetState(m_rasterizerState->GetRawRasterizerState());

        deviceContext->VSSetShader(m_shadowVS->GetRawShader(), nullptr, 0);
        deviceContext->PSSetShader(m_pickingPS->GetRawShader(), nullptr, 0);
        ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
        deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, &srv);

        deviceContext->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);
    }

    void SpriteRenderer::Refresh()
    {
        SetTexture(m_textureFilePath);

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