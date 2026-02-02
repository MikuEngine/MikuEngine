#include "EnginePCH.h"
#include "ParticleEmitter.h"

#include "Core/Graphics/Resource/ResourceManager.h"

namespace engine
{
	void ParticleEmitter::Initialize(const EmitterProps& props) 
	{
		m_textureFilePath = "Resource/Texture/DefaultParticle.png";
		m_texture = ResourceManager::Get().GetOrCreateTexture(m_textureFilePath);
		m_props = props;
		m_particles.reserve(props.maxParticles);
	}

	void ParticleEmitter::Update(float dt, const Vector3& emitterPosition, const Vector3& cameraPosition, bool isPlaying) 
	{
		// 재생 중일 때만 emitter age와 파티클 생성 처리
		if (isPlaying)
		{
			m_emitterAge += dt;

			for (auto& burst : m_props.bursts)
			{
				ProcessBurst(burst, m_emitterAge, emitterPosition);
			}

			m_emissionTimer += dt;
			float emissionInterval = 1.0f / std::max(EPSILON, m_props.emissionRate);

			while (m_emissionTimer >= emissionInterval)
			{
				Emit(emitterPosition + m_props.positionOffset);
				m_emissionTimer -= emissionInterval;
			}
		}

		for (auto& p : m_particles)
		{
			p.age += dt;
			if (p.age > p.lifeTime)
			{
				continue;
			}

			p.velocity += m_props.gravity * dt;
			p.position += p.velocity * dt;
			p.rotation += p.rotationSpeed * dt;

			float t = p.age / p.lifeTime;
			p.color = Vector4::Lerp(p.startColor, p.endColor, t);
			p.size = std::lerp(p.startSize, p.endSize, t);

			p.distanceToCamera = Vector3::DistanceSquared(p.position, cameraPosition);

			if (m_props.textureTilesX > 1 || m_props.textureTilesY > 1)
			{
				float totalTiles = static_cast<float>(m_props.textureTileCount > 0 ? 
					m_props.textureTileCount :
					(m_props.textureTilesX * m_props.textureTilesY));

				float t = p.age / p.lifeTime;

				t *= m_props.animationSpeed;

				// 현재 인덱스
				int index = static_cast<int>(t * totalTiles);
				index %= static_cast<int>(totalTiles);

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

		size_t aliveCount = m_particles.size();
		for (size_t i = 0; i < aliveCount; )
		{
			if (m_particles[i].age > m_particles[i].lifeTime)
			{
				--aliveCount;
				if (i < aliveCount)
				{
					std::swap(m_particles[i], m_particles[aliveCount]);
				}
			}
			else
			{
				++i;
			}
		}
		m_particles.resize(aliveCount);
	}

	void ParticleEmitter::Reset()
	{
		m_particles.clear();
		m_emissionTimer = 0.0f;
		m_emitterAge = 0.0f;

		for (auto& burst : m_props.bursts)
		{
			burst.currentCycle = 0;
			burst.nextBurstTime = 0.0f;
		}
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

	const std::string& ParticleEmitter::GetTexturePath() const
	{
		return m_textureFilePath;
	}

	void ParticleEmitter::SetProps(const EmitterProps& props) 
	{
		m_props = props;

		if (m_particles.capacity() < props.maxParticles)
		{
			m_particles.reserve(props.maxParticles);
		}
	}

	void ParticleEmitter::SetTexturePath(const std::string& filePath)
	{
		m_textureFilePath = filePath;

		if (!filePath.empty())
		{
			m_texture = ResourceManager::Get().GetOrCreateTexture(filePath);
		}
	}

	void ParticleEmitter::SortParticlesByDistance()
	{
		std::sort(m_particles.begin(), m_particles.end(),
			[](const Particle& a, const Particle& b)
			{
				return a.distanceToCamera > b.distanceToCamera;
			});
	}

	size_t ParticleEmitter::GetActiveParticleCount() const
	{
		return m_particles.size();
	}

	bool ParticleEmitter::IsFinished(bool isPlaying) const
	{
		// 재생 중이 아니면 (Stop 호출됨) 활성 파티클이 없으면 끝난 것으로 간주
		// Stop() 후에는 파티클이 자연스럽게 사라질 때까지 기다림
		if (!isPlaying)
		{
			return m_particles.size() == 0;
		}

		// 재생 중일 때는:
		// 1. 활성 파티클이 있으면 아직 끝나지 않음
		if (m_particles.size() > 0)
		{
			return false;
		}

		// 2. 활성 파티클이 없고, 더 이상 파티클을 생성하지 않을 때만 끝난 것으로 간주
		// emissionRate가 0이고, 모든 burst가 끝났는지 확인
		if (m_props.emissionRate <= 0.0f)
		{
			// 모든 burst가 완료되었는지 확인
			for (const auto& burst : m_props.bursts)
			{
				// cycles가 0이면 무한 반복이므로 끝나지 않음
				if (burst.cycles == 0)
				{
					return false;
				}
				// cycles가 설정되어 있으면 모든 cycle이 완료되었는지 확인
				// (실제로는 Update에서 처리되므로 여기서는 단순히 cycles가 0이 아니면 끝날 수 있다고 가정)
			}
			return true;
		}

		// emissionRate가 0보다 크면 계속 생성되므로 끝나지 않음
		return false;
	}

	void ParticleEmitter::Save(json& j) const
	{
		// Texture 경로
		j["TexturePath"] = m_textureFilePath;

		// EmitterProps 저장
		const auto& p = m_props;

		j["PositionOffset"] = p.positionOffset;
		j["Velocity"] = p.velocity;
		j["VelocityVariation"] = p.velocityVariation;
		j["Gravity"] = p.gravity;

		j["StartColor"] = p.startColor;
		j["EndColor"] = p.endColor;
		j["Blend"] = static_cast<int>(p.blend);

		j["SizeBegin"] = p.sizeBegin;
		j["SizeEnd"] = p.sizeEnd;
		j["SizeVariation"] = p.sizeVariation;
		j["LifeTime"] = p.lifeTime;

		j["EmissionRate"] = p.emissionRate;
		j["MaxParticles"] = p.maxParticles;

		j["Shape"] = static_cast<int>(p.shape);
		j["Radius"] = p.radius;
		j["Angle"] = p.angle;
		j["BoxSize"] = p.boxSize;
		j["RandomDirection"] = p.randomDirection;

		j["TextureTilesX"] = p.textureTilesX;
		j["TextureTilesY"] = p.textureTilesY;
		j["TextureTileCount"] = p.textureTileCount;
		j["AnimationSpeed"] = p.animationSpeed;
		j["IsRandomFrame"] = p.isRandomFrame;

		// Burst 배열 저장 (currentCycle, nextBurstTime 제외)
		j["Bursts"] = json::array();
		for (const auto& burst : p.bursts)
		{
			json burstJson;
			burstJson["Time"] = burst.time;
			burstJson["Count"] = burst.count;
			burstJson["Cycles"] = burst.cycles;
			burstJson["Interval"] = burst.interval;
			j["Bursts"].push_back(burstJson);
		}
	}

	void ParticleEmitter::Load(const json& j)
	{
		// Texture 경로 로드
		std::string texturePath;
		JsonGet(j, "TexturePath", texturePath);
		SetTexturePath(texturePath);

		// EmitterProps 로드
		auto& p = m_props;

		JsonGet(j, "PositionOffset", p.positionOffset);
		JsonGet(j, "Velocity", p.velocity);
		JsonGet(j, "VelocityVariation", p.velocityVariation);
		JsonGet(j, "Gravity", p.gravity);

		JsonGet(j, "StartColor", p.startColor);
		JsonGet(j, "EndColor", p.endColor);
		int blendInt = static_cast<int>(p.blend);
		JsonGet(j, "Blend", blendInt);
		p.blend = static_cast<EmitterBlend>(blendInt);

		JsonGet(j, "SizeBegin", p.sizeBegin);
		JsonGet(j, "SizeEnd", p.sizeEnd);
		JsonGet(j, "SizeVariation", p.sizeVariation);
		JsonGet(j, "LifeTime", p.lifeTime);

		JsonGet(j, "EmissionRate", p.emissionRate);
		JsonGet(j, "MaxParticles", p.maxParticles);

		int shapeInt = static_cast<int>(p.shape);
		JsonGet(j, "Shape", shapeInt);
		p.shape = static_cast<EmitterShape>(shapeInt);

		JsonGet(j, "Radius", p.radius);
		JsonGet(j, "Angle", p.angle);
		JsonGet(j, "BoxSize", p.boxSize);
		JsonGet(j, "RandomDirection", p.randomDirection);

		JsonGet(j, "TextureTilesX", p.textureTilesX);
		JsonGet(j, "TextureTilesY", p.textureTilesY);
		JsonGet(j, "TextureTileCount", p.textureTileCount);
		JsonGet(j, "AnimationSpeed", p.animationSpeed);
		JsonGet(j, "IsRandomFrame", p.isRandomFrame);

		// Burst 배열 로드
		p.bursts.clear();
		JsonArrayForEach(j, "Bursts", [&](const json& burstJson)
			{
				Burst burst;
				JsonGet(burstJson, "Time", burst.time);
				JsonGet(burstJson, "Count", burst.count);
				JsonGet(burstJson, "Cycles", burst.cycles);
				JsonGet(burstJson, "Interval", burst.interval);
				burst.currentCycle = 0;
				burst.nextBurstTime = 0.0f;
				p.bursts.push_back(burst);
			});

		// 파티클 풀 크기 조정
		m_particles.reserve(p.maxParticles);
	}

	void ParticleEmitter::Emit(const Vector3& emitterPosition) 
	{
		if (m_particles.size() >= m_props.maxParticles)
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

		if (m_props.shape != EmitterShape::Box || m_props.randomDirection)
		{
			float speed = p.velocity.Length();
			p.velocity = direction * speed;  // 방향 적용
		}

		p.velocity.x += Random::Float(-m_props.velocityVariation.x, m_props.velocityVariation.x);
		p.velocity.y += Random::Float(-m_props.velocityVariation.y, m_props.velocityVariation.y);
		p.velocity.z += Random::Float(-m_props.velocityVariation.z, m_props.velocityVariation.z);

		if (m_props.isRandomFrame && (m_props.textureTilesX > 1 || m_props.textureTilesY > 1))
		{
			int maxFrame = m_props.textureTileCount > 0 ?
				static_cast<int>(m_props.textureTileCount) :
				static_cast<int>(m_props.textureTilesX * m_props.textureTilesY);

			int randomFrame = Random::Int(0, maxFrame - 1);
			int col = randomFrame % m_props.textureTilesX;
			int row = randomFrame / m_props.textureTilesX;
			p.uvScale = Vector2(1.0f / m_props.textureTilesX, 1.0f / m_props.textureTilesY);
			p.uvOffset = Vector2(col * p.uvScale.x, row * p.uvScale.y);
		}
		else
		{
			p.uvOffset = Vector2(0.0f, 0.0f);
			p.uvScale = Vector2(1.0f / m_props.textureTilesX, 1.0f / m_props.textureTilesY);
		}

		p.startColor = m_props.startColor;
		p.endColor = m_props.endColor;

		p.rotation = Random::Float(0.0f, 360.0f);
		p.rotationSpeed = Random::Float(-45.0f, 45.0f);

		p.startSize = m_props.sizeBegin + Random::Float(-m_props.sizeVariation, m_props.sizeVariation);
		p.endSize = m_props.sizeEnd;
		p.size = p.startSize;

		p.age = 0.0f;
		p.lifeTime = m_props.lifeTime;

		m_particles.push_back(p);
	}

	void ParticleEmitter::ProcessBurst(Burst& burst, float emitterAge, const Vector3& emitterPosition)
	{
		bool shouldEmit = false;

		if (burst.currentCycle == 0 && emitterAge >= burst.time)
		{
			shouldEmit = true;
			burst.currentCycle = 1;
			burst.nextBurstTime = emitterAge + burst.interval;
		}
		else if (burst.currentCycle > 0 &&
			burst.currentCycle < burst.cycles &&
			emitterAge >= burst.nextBurstTime)
		{
			shouldEmit = true;
			burst.currentCycle++;
			burst.nextBurstTime = emitterAge + burst.interval;
		}
		else if (burst.cycles == 0 &&
			burst.currentCycle > 0 &&
			emitterAge >= burst.nextBurstTime)
		{
			shouldEmit = true;
			burst.nextBurstTime = emitterAge + burst.interval;
		}

		if (shouldEmit)
		{
			for (std::uint32_t i = 0; i < burst.count; ++i)
			{
				Emit(emitterPosition + m_props.positionOffset);
			}
		}
	}
}