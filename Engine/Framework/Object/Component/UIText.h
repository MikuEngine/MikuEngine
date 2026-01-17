#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/UIElement.h"

namespace engine
{
    class Texture;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class InputLayout;
    class VertexBuffer;
    class IndexBuffer;
    class SamplerState;
    class BlendState;
    class DepthStencilState;

    struct UIFont
    {
        std::shared_ptr<Texture> atlas;
        
    };

    // Text 정렬
    enum class UITextAlignH { Left, Center, Right };
    enum class UITextAlignV { Top, Middle, Bottom };

	class UIText : public UIElement
	{
        REGISTER_COMPONENT(UIText);
        
    private:
        std::string m_text = "Text";
        Vector4 m_color = Vector4(1, 1, 1, 1);

        std::string m_fontMetaPath;
        std::shared_ptr<UIFont> m_font;

        float m_fontScale = 1.0f;
        float m_letterSpacing = 0.0f;
        float m_lineSpacing = 0.0f;
        bool  m_wordWrap = false;

        // Default
        UITextAlignH m_alignH = UITextAlignH::Left;
        UITextAlignV m_alignV = UITextAlignV::Top;

        std::shared_ptr<VertexShader> m_vs;
        std::shared_ptr<PixelShader>  m_ps;
        std::shared_ptr<InputLayout>  m_inputLayout;
        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer>  m_indexBuffer;
        std::shared_ptr<SamplerState> m_sampler;
        std::shared_ptr<BlendState>   m_blend;
        std::shared_ptr<DepthStencilState> m_depthNone;
        std::shared_ptr<ConstantBuffer> m_uiCB;

        bool m_useAlphaBlend = true;
        bool m_drawOnlyWhenRectValid = true;

	public:
		UIText() = default;
		~UIText() override = default;

        void Initialize() override;
		void DrawUI() const override;

        void SetText(const std::string& text);
        const std::string& GetText() const;

        void SetColor(const Vector4& color);
        const Vector4& GetColor() const;

        void SetFont(const std::string& fontMetaPath);
        const std::string& GetFontPath() const;

        void SetAlphaBlend(bool enable);
        bool IsAlphaBlend() const;

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        DirectX::BoundingBox GetBounds() const override;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;

    private:
        void RefreshFont();
        //void DrawGlyphQuad(const UIRect& glyphRectPx, const Vector4& uv01, const Vector4& color) const;
	};
}


