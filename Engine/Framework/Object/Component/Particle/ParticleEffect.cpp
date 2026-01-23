#include "EnginePCH.h"
#include "ParticleEffect.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/System/ParticleSystem.h"
#include "Framework/Object/Component/Transform.h"
#include "Core/Graphics/Resource/ResourceManager.h"

namespace engine
{
	ParticleEffect::~ParticleEffect()
	{
		SystemManager::Get().GetParticleSystem().Unregister(this);
	}

	void ParticleEffect::Initialize()
	{
		SystemManager::Get().GetParticleSystem().Register(this);

		AddEmitter();
	}

	void ParticleEffect::Update()
	{
		if (!m_isPlaying)
		{
			return;
		}

		Vector3 camPos;
		if (auto mainCam = SystemManager::Get().GetCameraSystem().GetMainCamera())
		{
			camPos = mainCam->GetPosition();
		}

		float dt = Time::DeltaTime();

		Vector3 myPos = GetTransform()->GetWorldPosition();

		for (auto& emitter : m_emitters)
		{
			emitter.Update(dt, myPos, camPos);
		}
	}

	void ParticleEffect::AddEmitter()
	{
		auto emitter = ParticleEmitter();
		
		EmitterProps props; // 기본값 사용
		
		auto defaultTex = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/DefaultParticle.png");
		
		emitter.Initialize(defaultTex, props);

		m_emitters.push_back(emitter);
	}

	const std::vector<ParticleEmitter>& ParticleEffect::GetEmitters() const
	{
		return m_emitters;
	}

	void ParticleEffect::SortParticles()
	{
		for (auto& emitter : m_emitters)
		{
			emitter.SortParticlesByDistance();
		}
	}

	void ParticleEffect::Play()
	{
		m_isPlaying = true;
	}

	void ParticleEffect::Stop()
	{
		m_isPlaying = false;
	}
}