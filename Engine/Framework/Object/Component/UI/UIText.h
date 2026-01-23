#pragma once

#include "Framework/Object/Component/UI/UIElement.h"

namespace engine
{
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class InputLayout;
    class VertexBuffer;
    class IndexBuffer;
    class SamplerState;
    class BlendState;
    class DepthStencilState;
    class FontData;

    // Text 정렬
    enum class UITextAlignH { Left, Center, Right };
    enum class UITextAlignV { Top, Middle, Bottom };

	class UIText : public UIElement
	{
        REGISTER_COMPONENT(UIText, UIElement)
        
    public:
        UIText() = default;
        ~UIText() override = default;

        void Initialize() override;
		void DrawUI() const override;

    public:
        // Text
        void SetText(const std::string& utf8);
        const std::string& GetText() const;

        // Font
        void SetFontPath(const std::string& ttfPath);
        const std::string& GetFontPath() const;

        void SetFontPixelSize(int px);
        int GetFontPixelSize() const;

        // Style
        void SetColor(const Vector4& color);
        const Vector4& GetColor() const;

        void SetAlphaBlend(bool enable);
        bool IsAlphaBlend() const;

        //void SetAlignment(UITextAlignH alineH);
        //void SetAlignment(UITextAlignV alineV);

        // Spacing
        void SetLetterSpacing(float px);
        float GetLetterSpacing() const;

        void SetLineSpacing(float mul);
        float GetLineSpacing() const;

    public:
        bool HasRenderType(RenderType type) const override;
        void Draw(RenderType type) const override;
        DirectX::BoundingBox GetBounds() const override;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    private:
        void RefreshFont();

    private:
        std::string m_text = "UIText";
        std::string m_fontPath = "Resource/Font/malgun.ttf";
        int m_fontPixelSize = 32;

        std::shared_ptr<FontData> m_font;

        // Default
        UITextAlignH m_alignH = UITextAlignH::Left;
        UITextAlignV m_alignV = UITextAlignV::Top;

        std::string m_vsFilePath;
        std::string m_psFilePath;

        std::shared_ptr<VertexShader> m_vs;
        std::shared_ptr<PixelShader>  m_ps;
        std::shared_ptr<InputLayout>  m_inputLayout;
        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer>  m_indexBuffer;
        std::shared_ptr<SamplerState> m_sampler;
        std::shared_ptr<BlendState>   m_blend;
        std::shared_ptr<DepthStencilState> m_depthNone;
        std::shared_ptr<ConstantBuffer> m_uiCB;

        Vector4 m_color = Vector4(1, 1, 1, 1);
        bool m_useAlphaBlend = true;
        
        // Layout
        float m_letterSpacingPx = 0.0f;
        float m_lineSpacingMul = 1.0f;
	};
}