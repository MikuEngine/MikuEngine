#pragma once

#include "Framework/Object/Component/Transform.h"

namespace engine
{
	struct UIRect
	{
		float x = 0.0f;
		float y = 0.0f;
		float w = 0.0f;
		float h = 0.0f;

		Vector2 Pos() const { return { x, y }; }
		Vector2 Size() const { return { w, h }; }
	};

	class RectTransform :
		public Transform
	{
	private:
		Vector2 m_anchoredPosition{ 0.0f, 0.0f };
		
		float m_width = 100.0f;
		float m_height = 100.0f;

		Vector2 m_pivot{ 0.5f, 0.5f };

		Vector2 m_anchorMin{ 0.5f, 0.5f };
		Vector2 m_anchorMax{ 0.5f, 0.5f };

		UIRect m_worldRect{};
		bool m_uiDirty = true;

	public:
		RectTransform() = default;
		~RectTransform() override = default;

	public:
		// Getter
		const Vector2& GetAnchoredPosition() const { return m_anchoredPosition; }
		float GetWidth() const { return m_width; }
		float GetHeight() const { return m_height; }
		Vector2 GetSize() const { return { m_width, m_height }; }
		const Vector2& GetPivot() const { return m_pivot; }
		const Vector2& GetAnchorMin() const { return m_anchorMin; }
		const Vector2& GetAnchorMax() const { return m_anchorMax; }
		const UIRect& GetWorldRect() const { return m_worldRect; }

		bool IsUIDirty() const { return m_uiDirty; }
	public:
		// Setter
		void SetAnchoredPosition(const Vector2& p);
		void SetWidth(float w);
		void SetHeight(float h);
		void SetSize(float w, float h);
		void SetPivot(const Vector2& p);
		void SetAnchorMin(const Vector2& a);
		void SetAnchorMax(const Vector2& a);

		void MarkUIDirty(bool v = true) { m_uiDirty = v; }

	public:
		// Calculate
		void Recalculate(const UIRect& parentRect);

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};

}