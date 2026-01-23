#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Particle/ParticleEffect.h"

namespace engine
{
	class VertexBuffer;
	class IndexBuffer;
	class VertexShader;
	class PixelShader;
	class InputLayout;
	class BlendState;
	class DepthStencilState;

	class ParticleSystem :
		public System<ParticleEffect>
	{
	private:
		static constexpr UINT MAX_PARTICLES = 10000;

		std::shared_ptr<VertexBuffer> m_quadVB;
		std::shared_ptr<IndexBuffer> m_quadIB;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleSRV;

		std::shared_ptr<VertexShader> m_vs;
		std::shared_ptr<PixelShader> m_ps;
		std::shared_ptr<InputLayout> m_inputLayout;

		std::shared_ptr<BlendState> m_blendState;
		std::shared_ptr<DepthStencilState> m_dsState;

	public:
		ParticleSystem();

		void Update();
		void Render(const Matrix& view, const Matrix& projection);
	};
}