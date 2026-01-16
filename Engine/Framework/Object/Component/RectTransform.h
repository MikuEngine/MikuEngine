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
		const Vector2& GetAnchoredPosition();
		float GetWidth() const;
		float GetHeight() const;
		Vector2 GetSize() const;
		const Vector2& GetPivot() const;
		const Vector2& GetAnchorMin() const;
		const Vector2& GetAnchorMax() const;
		const UIRect& GetWorldRect() const;
		RectTransform* FindPrentRectTransform() const;

		bool IsUIDirty() const;

	public:
		// Setter
		void SetAnchoredPosition(const Vector2& pos);
		void SetWidth(float w);
		void SetHeight(float h);
		void SetSize(float w, float h);
		void SetPivot(const Vector2& pivot);
		void SetAnchorMin(const Vector2& anchorMin);
		void SetAnchorMax(const Vector2& anchorMax);

		void MarkUIDirty(bool v = true);

	public:
		// Calculate
		void Recalculate(const UIRect& parentRect);
		UIRect& GetWorldRectResolved(const UIRect& rootRect);

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};

}