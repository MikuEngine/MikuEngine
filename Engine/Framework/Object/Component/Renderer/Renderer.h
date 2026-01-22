#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Light.h"
#include "Framework/Asset/MaterialData.h"

namespace engine
{
	enum class RenderType
	{
		Shadow,
		Opaque,
		Cutout,
		Transparent,
		Screen,
		Count
	};

	class Renderer :
		public Component
	{
		DEFINE_COMPONENT_TYPE(Renderer, Component)

	private:
		std::array<std::int32_t, static_cast<size_t>(RenderType::Count)> m_systemIndices;

	public:
		Renderer();
		~Renderer();

	public:
		void Initialize() override;
		virtual void Update() {}

	public:
		virtual bool HasRenderType(RenderType type) const = 0;
		virtual void Draw(RenderType type) const = 0;
		virtual DirectX::BoundingBox GetBounds() const = 0;
		virtual bool IsCastShadow() const { return false; }

		virtual void DrawShadow(RenderType renderType, LightType lightType) const {}
		virtual void DrawMask() const {}
		virtual void DrawPickingID() const {}

	private:
		friend class RenderSystem;
	};
}