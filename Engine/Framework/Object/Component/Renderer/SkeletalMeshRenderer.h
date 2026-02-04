#pragma once

#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"

namespace engine
{
    class SkeletalMeshData;
    class SkeletonData;
    class MaterialData;
    class SkeletalAnimator;

    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class Texture;
    class InputLayout;
    class SamplerState;
    class RasterizerState;

    class SkeletalMeshRenderer :
        public Renderer
    {
        REGISTER_COMPONENT(SkeletalMeshRenderer, Renderer)

    private:
        std::shared_ptr<SkeletalMeshData> m_meshData;
        std::shared_ptr<SkeletonData> m_skeletonData;
        std::shared_ptr<MaterialData> m_materialData;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;

        std::shared_ptr<ConstantBuffer> m_materialConstantBuffer;
        std::shared_ptr<ConstantBuffer> m_objectConstantBuffer;      // WorldTransform, BoneIndex(Rigid)
        std::shared_ptr<ConstantBuffer> m_boneConstantBuffer;        // BoneTransforms

        std::shared_ptr<VertexShader> m_vs;
        std::shared_ptr<VertexShader> m_shadowVS;
        std::shared_ptr<VertexShader> m_spotShadowVS;
        std::shared_ptr<VertexShader> m_pointShadowVS;
        std::shared_ptr<VertexShader> m_simpleVS;

        std::shared_ptr<PixelShader> m_opaquePS;
        std::shared_ptr<PixelShader> m_cutoutPS;
        std::shared_ptr<PixelShader> m_transparentPS;
        std::shared_ptr<PixelShader> m_maskCutoutPS;
        std::shared_ptr<PixelShader> m_pickingPS;
        std::shared_ptr<PixelShader> m_pointShadowPS;
        std::shared_ptr<PixelShader> m_pointShadowCutoutPS;

        std::vector<Textures> m_textures;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;
        std::shared_ptr<RasterizerState> m_rasterizerState;

        CbBone m_boneTransformData; // CPU측 본 데이터

        std::string m_meshFilePath;
        std::string m_vsFilePath;
        std::string m_opaquePSFilePath;
        std::string m_cutoutPSFilePath;
        std::string m_transparentPSFilePath;

        Vector4 m_materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector3 m_materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
        float m_materialRoughness = 1.0f;
        float m_materialMetalness = 0.0f;
        float m_materialAmbientOcclusion = 1.0f;
        float m_materialEmissiveIntensity = 1.0f;
        bool m_overrideMaterial = false;
        bool m_castShadow = false;
        CullMode m_cullMode = CullMode::Back;
        float m_subsurfaceStrength = 0.0f;  // SSS strength for whole mesh (0 = off), GBuffer ORM.a. Later: texture modulates.
        Vector3 m_subsurfaceColor = Vector3(1.0f, 1.0f, 1.0f);  // SSS tint per renderer (RGB). (0,0,0) = use global CbFrame subsurfaceColor.

    public:
        SkeletalMeshRenderer();
        ~SkeletalMeshRenderer();

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

        void Initialize() override;
        void Awake() override;
        void Update();

        void SetMesh(const std::string& meshName);
        void SetVertexShader(const std::string& shaderFilePath);
        void SetOpaquePixelShader(const std::string& shaderFilePath);
        void SetCutoutPixelShader(const std::string& shaderFilePath);
        void SetTransparentPixelShader(const std::string& shaderFilePath);
        void SetCastShadow(bool cast);
        bool IsCastShadow() const override;
        void SetCullMode(CullMode cullMode);

        std::shared_ptr<SkeletonData> GetSkeletonData() const;
        const std::string& GetMeshPath() const override;

        // AfterimageRenderer 등에서 본 데이터 참조용 (Update() 이후 유효)
        const CbBone& GetBoneTransformData() const { return m_boneTransformData; }

        // 런타임 머티리얼 색상 조작
        void SetBaseColor(const Vector4& color) { m_materialBaseColor = color; }
        const Vector4& GetBaseColor() const { return m_materialBaseColor; }

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        void DrawShadow(RenderType renderType, LightType lightType) const override;
        DirectX::BoundingBox GetBounds() const override;
        void DrawMask() const override;
        void DrawPickingID() const override;

        void UpdateSockets() override;

    private:
        void Refresh();
    };
}