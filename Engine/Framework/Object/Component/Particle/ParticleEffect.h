#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Particle/ParticleEmitter.h"

namespace engine
{
	class ParticleEffect :
		public Component
	{
		REGISTER_COMPONENT(ParticleEffect, Component)

	private:
		std::vector<ParticleEmitter> m_emitters;
		bool m_isPlaying = true;

	public:
		~ParticleEffect();

		void Initialize() override;
		void Update();

		void AddEmitter();
		const std::vector<ParticleEmitter>& GetEmitters() const;

		void SortParticles();

		void Play();
		void Stop();
	};
}