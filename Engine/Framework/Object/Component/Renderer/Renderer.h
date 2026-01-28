#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Light/Light.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/Asset/SocketData.h"

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

	enum class CullMode
	{
		None,
		Back,
		Front
	};

	struct SocketInstance
	{
		const Socket* info = nullptr;
		Matrix worldMatrix;
	};

	class Renderer :
		public Component
	{
		DEFINE_COMPONENT_TYPE(Renderer, Component)

	private:
		std::array<std::int32_t, static_cast<size_t>(RenderType::Count)> m_systemIndices;

	protected:
		std::string m_emptyPath = "";
		std::shared_ptr<SocketData> m_socketData;
		std::vector<SocketInstance> m_socketInstances;

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

		void DrawSocketEditor();
		void SaveSocketData();
		void LoadSocketData();

		virtual void UpdateSockets();

		virtual Matrix GetSocketWorldMatrix(const std::string& name) const;
		const std::vector<SocketInstance>& GetSocketInstances() const { return m_socketInstances; }
		virtual const std::string& GetMeshPath() const { return m_emptyPath; }

		void ClearSockets();

	private:
		friend class RenderSystem;
	};
}