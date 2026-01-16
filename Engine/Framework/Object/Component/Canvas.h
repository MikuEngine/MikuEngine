#pragma once

#include "Framework/Object/Component/Component.h"

// Canvas는 UI트리의 루트 컴포넌트 입니다.

namespace engine
{
	class Canvas : public Component
	{
		REGISTER_COMPONENT(Canvas);
	private:
		Vector2 m_referenceResolution{ 1920.0f, 1080.0f };

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

	public:
		void ReclaulateLayout(float viewportW, float viewportH);

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};
}

