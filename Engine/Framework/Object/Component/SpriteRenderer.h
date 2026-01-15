#pragma once

#include "Framework/Object/Component/Renderer.h"

namespace engine
{
    class SimpleMeshData;

    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class Texture;
    class InputLayout;
    class SamplerState;
    class RasterizerState;

    enum class CullMode
    {
        None,
        Back,
        Front
    };

    enum class BillboardType
    {
        None,
        Spherical,   // 모든 축에서 카메라 바라보기
        Cylindrical, // Y축 회전만 (나무, 캐릭터 등)
        ViewPlaneAligned,
        ViewPlaneVertical
    };

    class SpriteRenderer :
        public Renderer
    {
        REGISTER_COMPONENT(SpriteRenderer)

    private:
        std::string m_textureFilePath;
        std::string m_vsFilePath;
        std::string m_opaquePSFilePath;
        std::string m_cutoutPSFilePath;
        std::string m_transparentPSFilePath;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;
        
        std::shared_ptr<ConstantBuffer> m_materialConstantBuffer;
        std::shared_ptr<ConstantBuffer> m_objectConstantBuffer;
        std::shared_ptr<ConstantBuffer> m_spriteConstantBuffer;

        std::shared_ptr<VertexShader> m_vs;
        std::shared_ptr<VertexShader> m_shadowVS;

        std::shared_ptr<PixelShader> m_opaquePS;
        std::shared_ptr<PixelShader> m_cutoutPS;
        std::shared_ptr<PixelShader> m_transparentPS;
        std::shared_ptr<PixelShader> m_maskCutoutPS;

        std::shared_ptr<Texture> m_texture;
        std::shared_ptr<InputLayout> m_inputLayout;
        std::shared_ptr<SamplerState> m_samplerState;

        std::shared_ptr<RasterizerState> m_rasterizerState;

        MaterialRenderType m_renderType = MaterialRenderType::Opaque;
        CullMode m_cullMode = CullMode::None;
        BillboardType m_billboardType = BillboardType::None;
        float m_width = 100.0f;
        float m_height = 100.0f;
        Vector4 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vector2 m_uvOffset{ 0.0f, 0.0f };
        Vector2 m_uvScale{ 1.0f, 1.0f };
        Vector2 m_pivot{ 0.5f, 0.5f };
        bool m_castShadow = false;
        bool m_isLoaded = false;

    public:
        ~SpriteRenderer();

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

    public:
        void Initialize() override;

        void SetTexture(const std::string& textureFilePath);
        void SetVertexShader(const std::string& shaderFilePath);
        void SetOpaquePixelShader(const std::string& shaderFilePath);
        void SetCutoutPixelShader(const std::string& shaderFilePath);
        void SetTransparentPixelShader(const std::string& shaderFilePath);
        void SetCastShadow(bool castShadow);
        void SetCullMode(CullMode cullMode);
        void SetSpriteInfo(const Vector2& offset, const Vector2& scale, const Vector2 pivot);
        void SetBillboardType(BillboardType type);

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        DirectX::BoundingBox GetBounds() const override;
        void DrawMask() const override;

    private:
        void Refresh();
        void ReplaceRenderSystem();
    };
}