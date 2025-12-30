#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
	enum class RenderType
	{
		Opaque,
		Transparent,
		Screen,
		Shadow,
		Count
	};

	class Renderer :
		public Component
	{
	private:
		std::array<std::int32_t, static_cast<size_t>(RenderType::Count)> m_systemIndices;

	public:
		Renderer();
		~Renderer();

	public:
		void Initialize() override;

	public:
		virtual bool HasRenderType(RenderType type) const = 0;
		virtual void Draw(/*RenderType type*/) const = 0;
		virtual DirectX::BoundingBox GetBounds() const = 0;

	private:
		friend class RenderSystem;
	};
}