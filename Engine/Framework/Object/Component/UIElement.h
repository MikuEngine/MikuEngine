#pragma once

#include "Framework/Object/Component/Renderer.h"

namespace engine
{
	class RectTransform;
	class Canvas;

	// UIElement는 UI 컴포넌트들의 공통 베이스입니다.
	class UIElement : public Renderer
	{
	public:
		UIElement() = default;
		~UIElement() override;

	public:
		void Initialize() override;
		void OnEnable() override;
		void OnDisable() override;

	public:
		bool HasRenderType(RenderType type) const override;
		void Draw(RenderType type) const override;
		DirectX::BoundingBox GetBounds() const override;

	public:
		int m_orderInLayer = 0;
		bool m_raycastTarget = true;

		std::int32_t m_systemIndex = -1;

		// Helper
		RectTransform* GetRectTransform() const;
		Canvas* GetCanvasInParent() const;

	protected:
		virtual void DrawUI() const = 0;
		bool CanDraw() const;
	};
}