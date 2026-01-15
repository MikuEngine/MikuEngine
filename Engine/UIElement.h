#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
	class RectTransform;

	// UIElement는 UI 컴포넌트들의 공통 베이스입니다.
	class UIElement : public Component
	{
	public:
		UIElement() = default;
		~UIElement() override = default;

	public:
		RectTransform* GetRectTransform() const;

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};
}