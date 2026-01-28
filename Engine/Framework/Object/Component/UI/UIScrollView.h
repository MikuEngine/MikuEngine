#pragma once

#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/Object/Component/UI/UIInteractable.h"

namespace engine
{
	class RectTransform;
	struct UIRect;
	class UISlider;

	class UIImage;
	class UIText;

	class UIScrollView : public UIElement, public UIInteractable
	{
		REGISTER_COMPONENT(UIScrollView, UIElement)
	public:
		struct ScrollInfo
		{
			float scrollY = 0.0f;
			float maxScroll = 0.0f;
			float normalized = 0.0f;
		};

		using ScrollChangedCallback = std::function<void(const ScrollInfo&)>;
		using DragCallback = std::function<void()>;

		UIScrollView() = default;
		~UIScrollView() override = default;

		void Initialize() override;
		void DrawUI() const override {}

	public:
		void SetContentByName(const std::string& childName);
		void SetScrollbarByName(const std::string& goName);

		void SetScrollY(float y, bool syncScrollbar = true);
		float GetScrollY() const { return m_scrollY; }
		float GetMaxScroll() const;

		UIRect GetViewPortWorldRect() const;
		RectTransform* GetViewportRT() const { return m_viewportRT; }

	public:
		void AddOnScrollChanged(ScrollChangedCallback cb) { m_onScrollChanged.push_back(std::move(cb)); }
		void AddOnBeginDrag(DragCallback cb) { m_onBeginDrag.push_back(std::move(cb)); }
		void AddOnEndDrag(DragCallback cb) { m_onEndDrag.push_back(std::move(cb)); }

	public:
		bool IsScrollEnabled() const override { return true; }
		void OnScroll(const Vector2& mousePos, float wheelDelta) override;
		bool HitTestPoint(const Vector2& p) const override;

	private:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
		void ApplyLayout();

		void BindScrollbarCallBack();
		void ApplyContentPosition();
		void SyncScrollbarFromScroll();
		void EmitScrollChanged();
		void RebuildRendererCache();
		void ApplyClipToCachedRenderers(const Vector4& clipPx);
		void RefreshClipFromViewport(bool force);

		RectTransform* FindChildRTByName(const char* name) const;

	private:
		RectTransform* m_viewportRT = nullptr;
		RectTransform* m_contentRT = nullptr;
		UISlider* m_scrollbar = nullptr;

		float m_scrollY = 0.0f;
		float m_lastScrollbarV = 0.0f;
		float m_scrollbarDragSpeed = 1.0f;	// 스크롤 감도

		float m_contentHeight = 500.0f;
		float m_scrollbarWidth = 20.0f;
		float m_scrollbarGap = 0.0f;     // 뷰포트-바 간격

		Vector2 m_viewportSize = { 500.0f, 500.0f };

		std::string m_contentName = "Content";
		std::string m_viewportName = "Viewport";
		std::string m_scrollbarName;

		bool m_syncGuard = false;

		float m_cachedMaxScroll = -1.0f;

		std::vector<ScrollChangedCallback> m_onScrollChanged;
		std::vector<DragCallback> m_onBeginDrag;
		std::vector<DragCallback> m_onEndDrag;

		std::vector<UIImage*> m_cachedImages;
		std::vector<UIText*>  m_cachedTexts;

		Vector4 m_cachedClipPx = Vector4(-1, -1, -1, -1);
		bool m_rendererCacheDirty = true;
	};
}
