#pragma once

#include "Framework/Object/Component/Renderer/Renderer.h"

namespace engine
{
	class RectTransform;
	class Canvas;

	enum class UILayer
	{
		Background,
		Content,
		Overlay,
		Popup,
		Debug,
	};

	enum class MaskMode
	{
		None,
		Rect,
		Circle,
		Ring,
		RectRing,
		Radial,
	};

	// UIElement는 UI 컴포넌트들의 공통 베이스입니다.
	class UIElement : public Renderer
	{
		DEFINE_COMPONENT_TYPE(UIElement, Renderer)

	public:
		UIElement() = default;
		~UIElement() override;

	public:
		void Initialize() override;
		void OnEnable() override;
		void OnDisable() override;

		virtual bool HitTestPoint(const Vector2& screenPos) const;

	public:
		bool HasRenderType(RenderType type) const override;
		void Draw(RenderType type) const override;
		DirectX::BoundingBox GetBounds() const override;

	public:
		int m_layer = 0;
		int m_orderInLayer = 0;
		bool m_raycastTarget = true;

		std::int32_t m_systemIndex = -1;
		std::uint64_t m_registerSerial = 0;

		void SetLayer(int layer);
		void SetOrderInLayer(int order);

		int GetLayer();
		int GetOrderInLayer();

	public:
		// Helper
		RectTransform* GetRectTransform() const;
		Canvas* GetCanvasInParent() const;

	protected:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

		virtual void DrawUI() const = 0;
		bool CanDraw() const;

		// helper
		float Clamp01(float v);
	};
}