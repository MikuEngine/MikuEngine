#pragma once

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

	class UIImage : public UIElement
	{
		REGISTER_COMPONENT(UIImage)
	private:
		std::string m_textureFilePath;
		std::string m_vsFilePath;
		std::string m_psFilePath;
		std::shared_ptr<Texture> m_texture;

		std::shared_ptr<VertexShader> m_vs;
		std::shared_ptr<PixelShader>  m_ps;
		std::shared_ptr<InputLayout> m_inputLayout;

		std::shared_ptr<VertexBuffer> m_vertexBuffer;
		std::shared_ptr<IndexBuffer> m_indexBuffer;

		std::shared_ptr<SamplerState> m_sampler;
		std::shared_ptr<BlendState> m_blend;
		std::shared_ptr<DepthStencilState> m_depthNone;

		std::shared_ptr<ConstantBuffer> m_uiCB;

		Vector4 m_color = Vector4(1, 1, 1, 1);

		bool m_useAlphaBlend = true;
		bool m_drawOnlyWhenRectValid = true;

	public:
		UIImage() = default;
		~UIImage() override = default;

	public:
		void Initialize() override;
		void DrawUI() const override;

	public:
		void SetTexture(const std::string& textureFilePath);
		const std::string& GetTexturePath() const;

		void SetAlphaBlend(bool enable);
		bool IsAlphaBlend() const;
		
		void SetColor(const Vector4& color);
		const Vector4& GetColor() const;

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
		void Refresh();
	};
}