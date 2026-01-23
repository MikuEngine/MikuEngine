#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Particle/ParticleEffect.h"
#include "Core/Graphics/Data/ParticleStructuredData.h"

namespace engine
{
	class VertexBuffer;
	class IndexBuffer;
	class VertexShader;
	class PixelShader;
	class InputLayout;
	class BlendState;
	class DepthStencilState;
	struct TransparentRenderItem;  // Forward declaration (정의는 RenderSystem.h에 있음)

	class ParticleSystem :
		public System<ParticleEffect>
	{
		struct EffectEmitterPair
		{
			ParticleEffect* effect;
			const ParticleEmitter* emitter;
			float distanceSq;
		};


	private:
		static constexpr UINT MAX_PARTICLES = 10000;

		std::shared_ptr<VertexBuffer> m_quadVB;
		std::shared_ptr<IndexBuffer> m_quadIB;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleSRV;

		std::vector<ParticleStructuredData> m_structuredDataBuffer;
		std::vector<EffectEmitterPair> m_sortedPairs;

		std::shared_ptr<VertexShader> m_vs;
		std::shared_ptr<PixelShader> m_ps;
		std::shared_ptr<InputLayout> m_inputLayout;

		std::shared_ptr<BlendState> m_blendState;
		std::shared_ptr<DepthStencilState> m_dsState;

	public:
		ParticleSystem();

		void Update();
		void Render(const Matrix& view, const Matrix& projection);

		// 투명 렌더링 통합을 위한 함수
		void CollectTransparentItems(std::vector<TransparentRenderItem>& items, const Vector3& cameraPos);
		void RenderEmitter(ParticleEffect* effect, const ParticleEmitter* emitter);
	};
}