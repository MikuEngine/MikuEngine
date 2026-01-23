#pragma once

#include "Framework/Object/Component/Component.h"

// Canvas는 UI트리의 루트 컴포넌트 입니다.

namespace engine
{
	class Canvas : public Component
	{
		REGISTER_COMPONENT(Canvas, Component)

	private:
		Vector2 m_referenceResolution{ 1920.0f, 1080.0f };

		int m_resolutionPreset = 1;

		Vector2 m_uiScale{ 1.0f, 1.0f };   // 보통 {scale, scale}
		Vector2 m_uiOffset{ 0.0f, 0.0f };  // 레터박스 오프셋(픽셀)

		// Canvas 오브젝트의 RectTransform은 인스펙터에서 조절할 수 없게 합니다
		bool m_lockRectTransformInEditor = true;
		int m_sortingOrder = 0;

	public:
		Canvas() = default;
		~Canvas() override = default;

	public:
		const Vector2& GetReferenceResolution() const;
		void SetReferenceResolution(const Vector2& resolution);

		bool IsRectTransformLockedInEditor() const;
		void SetRectTransformLockedInEditor(bool lock);

		int GetSortingOrder() const;
		void SetSortingOrder(int order);

		const Vector2& GetUIScale() const { return m_uiScale; }
		const Vector2& GetUIOffset() const { return m_uiOffset; }

	public:
		void ReclaulateLayout(float viewportW, float viewportH);

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
	};
}

