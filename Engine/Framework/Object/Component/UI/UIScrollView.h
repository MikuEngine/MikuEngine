#pragma once

#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/Object/Component/UI/UIInteractable.h"

namespace engine
{
	class RectTransform;
	class UISlider;

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
		void Update() override;
		void DrawUI() const override;

	public:
		void SetContentByName(const std::string& childName);
		void SetScrollbarByName(const std::string& goName);

		void SetDragSpeed(float s) { m_dragSpeed = s; }

		void SetScrollY(float y, bool syncScrollbar = true);
		float GetScrollY() const { return m_scrollY; }
		float GetMaxScroll() const;

	public:
		bool IsInteractable() const override { return true; }
		bool IsDragEnabled() const override { return true; }

		void OnBeginDrag(const Vector2& mousePos, int mouseButton) override;
		void OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton) override;
		void OnEndDrag(const Vector2& mousePos, int mouseButton) override;
		void OnMouseCancel(const Vector2& mousePos, int mouseButton) override;

	public:
		void AddOnScrollChanged(ScrollChangedCallback cb) { m_onScrollChanged.push_back(std::move(cb)); }
		void AddOnBeginDrag(DragCallback cb) { m_onBeginDrag.push_back(std::move(cb)); }
		void AddOnEndDrag(DragCallback cb) { m_onEndDrag.push_back(std::move(cb)); }

	private:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
		void BindScrollbarCallBack();
		void ApplyContentPosition();
		void SyncScrollbarFromScroll();
		void EmitScrollChanged();

		RectTransform* FindChildRTByName(const char* name) const;

	private:
		RectTransform* m_viewportRT = nullptr;
		RectTransform* m_contentRT = nullptr;
		UISlider* m_scrollbar = nullptr;

		bool m_dragging = false;

		float m_scrollY = 0.0f;
		float m_dragSpeed = 1.0f;

		float m_lastScrollbarV = 0.0f;
		float m_scrollbarDragSpeed = 1.0f;	// 스크롤 감도

		std::string m_contentName = "Content";
		std::string m_scrollbarName;

		bool m_syncGuard = false;

		float m_cachedMaxScroll = -1.0f;

		std::vector<ScrollChangedCallback> m_onScrollChanged;
		std::vector<DragCallback> m_onBeginDrag;
		std::vector<DragCallback> m_onEndDrag;
	};
}
