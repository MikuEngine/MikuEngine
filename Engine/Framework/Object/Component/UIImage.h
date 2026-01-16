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

	class UIImage : public UIElement
	{
		REGISTER_COMPONENT(UIImage);
	private:
		std::string m_textureFilePath;
		std::string m_vsFilePath;
		std::string m_opaquePSFilePath;
		std::string m_cutoutPSFilePath;
		std::string m_transparentPSFilePath;

		std::shared_ptr<Texture> m_texture;

		std::shared_ptr<VertexShader> m_vs;

		std::shared_ptr<PixelShader> m_opaquePS;
		std::shared_ptr<PixelShader> m_cutoutPS;
		std::shared_ptr<PixelShader> m_transparentPS;
		std::shared_ptr<PixelShader> m_maskCutoutPS;

		std::shared_ptr<VertexBuffer> m_vertexBuffer;
		std::shared_ptr<IndexBuffer> m_indexBuffer;
		std::shared_ptr<InputLayout> m_inputLayout;

		std::shared_ptr<SamplerState> m_samplerState;
		std::shared_ptr<BlendState> m_blend;
		std::shared_ptr<DepthStencilState> m_depthNone;


		bool m_useAlphaBlend = true;
		bool m_drawOnlyWhenRectValid = true;
		bool m_isLoaded = false;

		float m_width = 100.0f;
		float m_height = 100.0f;

		MaterialRenderType m_renderType = MaterialRenderType::Transparent;

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
		void ReplaceRenderSystem();
		void Refresh();
	};
}