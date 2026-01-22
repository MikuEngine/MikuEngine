#include "EnginePCH.h"
#include "ParticleEmitter.h"

namespace engine
{
	void ParticleEmitter::Initialize(const std::shared_ptr<Texture>& texture, const EmitterProps& props) 
	{
		m_texture = texture;
		m_props = props;
		m_particles.reserve(props.maxParticle);
	}

	void ParticleEmitter::Update(float dt, const Vector3& emitterPosition, const Vector3& cameraPosition) 
	{
		m_emissionTimer += dt;
		float emissionInterval = 1.0f / std::max(EPSILON, m_props.emissionRate);

		while (m_emissionTimer >= emissionInterval)
		{
			Emit(emitterPosition + m_props.positionOffset);
			m_emissionTimer -= emissionInterval;
		}

		for (auto& p : m_particles)
		{
			p.age += dt;
			if (p.age > p.lifeTime)
			{
				continue;
			}

			p.velocity += Vector3(0.0f, -9.8f, 0.0f) * 0.0f * dt;
			p.position += p.velocity * dt;
			p.rotation += p.rotationSpeed * dt;

			float t = p.age / p.lifeTime;
			p.color = Vector4::Lerp(
				Vector4(p.startColor.x, p.startColor.y, p.startColor.z, 1.0f),
				Vector4(p.endColor.x, p.endColor.y, p.endColor.z, 0.0f), t);
			p.size = std::lerp(p.startSize, p.endSize, t);

			p.distanceToCamera = Vector3::DistanceSquared(p.position, cameraPosition);
		}

		auto iter = std::remove_if(m_particles.begin(), m_particles.end(),
			[](const Particle& p) { return p.age > p.lifeTime; });
		m_particles.erase(iter, m_particles.end());
	}

	const std::vector<Particle>& ParticleEmitter::GetParticles() const 
	{
		return m_particles;
	}

	const std::shared_ptr<Texture>& ParticleEmitter::GetTexture() const 
	{
		return m_texture;
	}

	const EmitterProps& ParticleEmitter::GetProps() const 
	{
		return m_props;
	}

	void ParticleEmitter::SetProps(const EmitterProps& props) 
	{
		m_props = props;
	}

	void ParticleEmitter::Emit(const Vector3& emitterPosition) 
	{
		if (m_particles.size() >= m_props.maxParticle)
		{
			return;
		}

		Particle p;

		p.position = emitterPosition;
		p.velocity = m_props.velocity;
		p.velocity.x += Random::Float(-m_props.velocityVariation.x, m_props.velocityVariation.x);
		p.velocity.y += Random::Float(-m_props.velocityVariation.y, m_props.velocityVariation.y);
		p.velocity.z += Random::Float(-m_props.velocityVariation.z, m_props.velocityVariation.z);

		p.startColor = Vector3(m_props.colorBegin);
		p.endColor = Vector3(m_props.colorEnd);

		p.rotation = Random::Float(0.0f, 360.0f);
		p.rotationSpeed = Random::Float(-45.0f, 45.0f);

		p.age = 0.0f;
		p.lifeTime = m_props.lifeTime;

		m_particles.push_back(p);
	}
}