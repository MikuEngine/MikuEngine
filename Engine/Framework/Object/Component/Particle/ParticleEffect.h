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
		bool m_autoDestroy = false;
		float m_duration = 0.0f; // 0 = 무한 반복, >0 = 해당 시간 후 자동 Stop
		float m_playTime = 0.0f; // 현재 재생 시간
		Vector3 m_prevWorldPos{};
		bool m_hasPrevPos = false;

	public:
		~ParticleEffect();

		static void* operator new(size_t size);
		static void operator delete(void* ptr);

		void Initialize() override;
		void Update();

		void AddEmitter();
		const std::vector<ParticleEmitter>& GetEmitters() const;

		void SortParticles();

		void Play();
		void Stop();

		// 파티클 효과가 완전히 끝났는지 확인
		bool IsFinished() const;

		// 자동 삭제 설정
		void SetAutoDestroy(bool autoDestroy);
		bool GetAutoDestroy() const;

		// Duration 설정 (0 = 무한 반복, >0 = 해당 시간 후 자동 Stop)
		void SetDuration(float duration);
		float GetDuration() const;

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
        void DrawEmitterGui(ParticleEmitter& emitter, int index);
	};
}