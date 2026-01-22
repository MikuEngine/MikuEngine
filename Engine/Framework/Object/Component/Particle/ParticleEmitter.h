#pragma once

namespace engine
{
	struct Particle
	{
		Vector3 position;
		Vector3 velocity;
		Vector4 color;
		Vector3 startColor;
		Vector3 endColor;

		float size;
		float startSize;
		float endSize;

		float rotation;
		float rotationSpeed;

		float age;
		float lifeTime;
		float distanceToCamera;
	};

	struct EmitterProps
	{
		Vector3 positionOffset{ 0.0f, 0.0f, 0.0f };

		Vector3 velocity{ 0.0f, 1.0f, 0.0f };
		Vector3 velocityVariation{ 0.5f, 0.5f, 0.5f };

		Vector4 colorBegin{ 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };
		float sizeBegin = 0.5f;
		float sizeEnd = 0.1f;
		float sizeVariation = 0.2f;
		float lifeTime = 1.0f;

		float emissionRate = 10.0f;
		std::uint32_t maxParticle = 1000;

		std::uint32_t textureTilesX = 1;
		std::uint32_t textureTilesY = 1;
	};

	class Texture;

	class ParticleEmitter
	{
	private:
		std::vector<Particle> m_particles;
		EmitterProps m_props;
		std::shared_ptr<Texture> m_texture;

		float m_emissionTimer = 0.0f;

	public:
		void Initialize(const std::shared_ptr<Texture>& texture, const EmitterProps& props);
		void Update(float dt, const Vector3& emitterPosition, const Vector3& cameraPosition);

		const std::vector<Particle>& GetParticles() const;
		const std::shared_ptr<Texture>& GetTexture() const;
		const EmitterProps& GetProps() const;
		void SetProps(const EmitterProps& props);

	private:
		void Emit(const Vector3& emitterPosition);
	};
}