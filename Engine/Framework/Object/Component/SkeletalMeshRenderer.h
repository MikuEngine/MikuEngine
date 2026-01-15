#pragma once

#include "Framework/Object/Component/Renderer.h"
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

    class SkeletalMeshRenderer :
        public Renderer
    {
        REGISTER_COMPONENT(SkeletalMeshRenderer)

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
        std::shared_ptr<VertexShader> m_simpleVS;

        std::shared_ptr<PixelShader> m_opaquePS;
        std::shared_ptr<PixelShader> m_cutoutPS;
        std::shared_ptr<PixelShader> m_transparentPS;
        std::shared_ptr<PixelShader> m_maskCutoutPS;

        std::vector<Textures> m_textures;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;

        CbBone m_boneTransformData; // CPU측 본 데이터

        std::string m_meshFilePath;
        std::string m_vsFilePath;
        std::string m_opaquePSFilePath;
        std::string m_cutoutPSFilePath;
        std::string m_transparentPSFilePath;

        Vector4 m_materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector3 m_materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
        float m_materialRoughness = 0.0f;
        float m_materialMetalness = 0.0f;
        float m_materialAmbientOcclusion = 1.0f;
        bool m_overrideMaterial = false;


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

        std::shared_ptr<SkeletonData> GetSkeletonData() const;
        const std::string& GetMeshPath() const;

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;

        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        DirectX::BoundingBox GetBounds() const override;
        void DrawMask() const override;

    private:
        void Refresh();
    };
}