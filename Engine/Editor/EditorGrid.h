#pragma once

namespace engine
{
	class VertexBuffer;
	class IndexBuffer;
	class VertexShader;
	class PixelShader;
	class DepthStencilState;
	class BlendState;
	class ConstantBuffer;
	class InputLayout;
	class SamplerState;
	class RasterizerState;

	class EditorGrid
	{
	private:
		std::shared_ptr<VertexBuffer> m_gridPlaneVB;
		std::shared_ptr<IndexBuffer> m_gridPlaneIB;

		std::shared_ptr<VertexShader> m_gridVS;
		std::shared_ptr<PixelShader> m_gridPS;

		std::shared_ptr<DepthStencilState> m_dss;
		std::shared_ptr<BlendState> m_bs;
		std::shared_ptr<ConstantBuffer> m_objectCB;
		std::shared_ptr<ConstantBuffer> m_gridCB;
		std::shared_ptr<InputLayout> m_gridInputLayout;
		std::shared_ptr<SamplerState> m_samplerLinear;
		std::shared_ptr<RasterizerState> m_rss;

	public:
		void Initialize();
		void Draw() const;
	};
}