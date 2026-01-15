#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/UIElement.h"

namespace engine
{
	class Texture;
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
		// Texture
		std::string m_textureFilePath;
		std::shared_ptr<Texture> m_texture;

		// Shader
		std::shared_ptr<VertexShader> m_vs;
		std::shared_ptr<PixelShader> m_ps;

		// Geometry
		std::shared_ptr<VertexBuffer> m_vertexBuffer;
		std::shared_ptr<IndexBuffer> m_indexBuffer;
		std::shared_ptr<InputLayout> m_inputLayout;

		// State
		std::shared_ptr<SamplerState> m_sampler;
		std::shared_ptr<BlendState> m_blend;
		std::shared_ptr<DepthStencilState> m_depthNone;

		// Options
		bool m_useAlphaBlend = true;
		bool m_drawOnlyWhenRectValid = true;

	public:
		UIImage() = default;
		~UIImage() override = default;

	public:
		void Initialize() override;
		void DrawUI() const override;

	public:
		void SetTexture(const std::string& filePath);
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
	};
}