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
		~UIElement() override = default;

	public:
		bool HasRenderType(RenderType type) const override;
		void Draw(RenderType type) const override;
		DirectX::BoundingBox GetBounds() const override;

	protected:
		virtual void DrawUI() const = 0;

		// Helper
		RectTransform* GetRectTransform() const;
		Canvas* GetCanvasInParent() const;

		bool CanDraw() const;

	public:
		void OnGui() override;
		virtual std::string GetType() const override;
	};
}