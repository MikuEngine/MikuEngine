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

			p.velocity += Vector3(0.0f, -9.8f, 0.0f) * /*0.0f **/ dt;
			p.position += p.velocity * dt;
			p.rotation += p.rotationSpeed * dt;

			float t = p.age / p.lifeTime;
			p.color = Vector4::Lerp(p.startColor, p.endColor, t);
			p.size = std::lerp(p.startSize, p.endSize, t);

			p.distanceToCamera = Vector3::DistanceSquared(p.position, cameraPosition);

			if (m_props.textureTilesX > 1 || m_props.textureTilesY > 1)
			{
				float totalTiles = (float)(m_props.textureTilesX * m_props.textureTilesY);

				// 정규화된 시간 (0.0 ~ 1.0)
				float t = p.age / p.lifeTime;

				// 애니메이션 속도 반영 (반복 등)
				t *= m_props.animationSpeed;

				if (m_props.isRandomFrame) {
					// 랜덤 프레임 로직 (필요시 구현)
				}

				// 현재 인덱스
				int index = static_cast<int>(t * totalTiles);
				index %= static_cast<int>(totalTiles); // Wrap around

				// Row / Col 계산
				int col = index % m_props.textureTilesX;
				int row = index / m_props.textureTilesX;
				p.uvScale.x = 1.0f / m_props.textureTilesX;
				p.uvScale.y = 1.0f / m_props.textureTilesY;

				p.uvOffset.x = static_cast<float>(col) * p.uvScale.x;
				p.uvOffset.y = static_cast<float>(row) * p.uvScale.y;
			}
			else
			{
				p.uvScale = Vector2(1.0f, 1.0f);
				p.uvOffset = Vector2(0.0f, 0.0f);
			}
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

		Particle p{};

		Vector3 spawnPos{};
		Vector3 direction{ 0.0f, 1.0f, 0.0f };

		switch (m_props.shape)
		{
		case EmitterShape::Sphere:
		{
			Vector3 rnd = Random::InsideUnitSphere();
			spawnPos = rnd * m_props.radius;

			if (m_props.randomDirection)
			{
				direction = Random::OnUnitSphere();
			}
			else
			{
				direction = rnd;
			}
		}
			break;

		case EmitterShape::Cone:
		{
			spawnPos = Random::InsideUnitCircle() * m_props.radius;

			float rad = ToRadian(m_props.angle);

			Vector3 up{ 0.0f, 1.0f, 0.0f };
			Vector3 outward = spawnPos;
			if (outward.LengthSquared() > 0.001f)
			{
				outward.Normalize();
			}
			else
			{
				outward = Vector3(1.0f, 0.0f, 0.0f);
			}

			direction = up + outward * std::tan(rad);
			direction.Normalize();
		}
			break;

		case EmitterShape::Box:
			spawnPos.x = Random::Float(-0.5f, 0.5f) * m_props.boxSize.x;
			spawnPos.y = Random::Float(-0.5f, 0.5f) * m_props.boxSize.y;
			spawnPos.z = Random::Float(-0.5f, 0.5f) * m_props.boxSize.z;

			direction = Vector3(0.0f, 1.0f, 0.0f);
			break;
		}


		p.position = emitterPosition + m_props.positionOffset + spawnPos;

		p.velocity = m_props.velocity;
		p.velocity.x += Random::Float(-m_props.velocityVariation.x, m_props.velocityVariation.x);
		p.velocity.y += Random::Float(-m_props.velocityVariation.y, m_props.velocityVariation.y);
		p.velocity.z += Random::Float(-m_props.velocityVariation.z, m_props.velocityVariation.z);

		p.uvOffset = Vector2(0.0f, 0.0f);
		p.uvScale = Vector2(1.0f / m_props.textureTilesX, 1.0f / m_props.textureTilesY);

		p.startColor = m_props.colorBegin;
		p.endColor = m_props.colorEnd;

		p.rotation = Random::Float(0.0f, 360.0f);
		p.rotationSpeed = Random::Float(-45.0f, 45.0f);

		p.startSize = m_props.sizeBegin + Random::Float(-m_props.sizeVariation, m_props.sizeVariation);
		p.endSize = m_props.sizeEnd;
		p.size = p.startSize;

		p.age = 0.0f;
		p.lifeTime = m_props.lifeTime;

		m_particles.push_back(p);
	}
}