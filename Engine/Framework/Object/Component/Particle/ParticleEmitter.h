#pragma once

namespace engine
{
	enum class EmitterShape
	{
		Sphere,
		Cone,
		Box
	};

	enum class EmitterBlend
	{
		Additive,
		Blend,
	};

	struct Particle
	{
		Vector3 position;
		float age;
		float lifeTime;
		float distanceToCamera;

		Vector3 velocity;
		float size;

		Vector4 color;
		Vector4 startColor;
		Vector4 endColor;

		Vector3 emissiveColor;
		float emissiveIntensity;

		float rotation;
		float rotationSpeed;

		Vector2 uvOffset;
		Vector2 uvScale;

		float startSize;
		float endSize;
	};

	struct Burst
	{
		float time = 0.0f;
		std::uint32_t count = 10;
		std::uint32_t cycles = 1;
		float interval = 0.0f;

		std::uint32_t currentCycle = 0;
		float nextBurstTime = 0.0f;
	};

	struct EmitterProps
	{
		Vector3 positionOffset{ 0.0f, 0.0f, 0.0f };

		Vector3 velocity{ 0.0f, 5.0f, 0.0f };
		Vector3 velocityVariation{ 1.0f, 1.0f, 1.0f };

		Vector3 gravity{ 0.0f, -9.8f, 0.0f };

		Vector4 startColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 endColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		Vector3 startEmissive{ 0.0f, 0.0f, 0.0f };
		Vector3 endEmissive{ 0.0f, 0.0f, 0.0f };
		float startEmissiveIntensity = 0.0f;
		float endEmissiveIntensity = 0.0f;
		float sizeBegin = 1.0f;
		float sizeEnd = 1.0f;
		float sizeVariation = 0.1f;
		float lifeTime = 2.0f;

		float rotationMin = 0.0f;
		float rotationMax = 360.0f;
		float rotationSpeedMin = -45.0f;
		float rotationSpeedMax = 45.0f;

		float emissionRate = 10.0f;
		std::uint32_t maxParticles = 1000;

		std::vector<Burst> bursts;

		EmitterShape shape = EmitterShape::Cone;
		float radius = 1.0f;
		float angle = 25.0f;
		Vector3 boxSize{ 1.0f, 1.0f, 1.0f };
		bool randomDirection = false;

		std::uint32_t textureTilesX = 1;
		std::uint32_t textureTilesY = 1;
		std::uint32_t textureTileCount = 1;
		float animationSpeed = 1.0f;
		bool isRandomFrame = false;

		EmitterBlend blend = EmitterBlend::Additive;
	};

	class Texture;

	class ParticleEmitter
	{
	private:
		std::vector<Particle> m_particles;
		EmitterProps m_props;
		std::shared_ptr<Texture> m_texture;
		std::string m_textureFilePath;

		float m_emissionTimer = 0.0f;
		float m_emitterAge = 0.0f;

	public:
		void Initialize(const EmitterProps& props);
		void Update(float dt, const Vector3& emitterPosition, const Vector3& cameraPosition, bool isPlaying = true);
		void Reset();

		const std::vector<Particle>& GetParticles() const;
		const std::shared_ptr<Texture>& GetTexture() const;
		const EmitterProps& GetProps() const;
		const std::string& GetTexturePath() const;

		void SetProps(const EmitterProps& props);
		void SetTexturePath(const std::string& filePath);

		void SortParticlesByDistance();

		// 파티클 상태 확인
		size_t GetActiveParticleCount() const;
		bool IsFinished(bool isPlaying) const;

	public: // 직렬화
		void Save(json& j) const;
		void Load(const json& j);

	private:
		void Emit(const Vector3& emitterPosition);
		void ProcessBurst(Burst& burst, float emitterAge, const Vector3& emitterPosition);
	};
}