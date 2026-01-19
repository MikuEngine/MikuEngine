#pragma once

#include "Framework/Object/Component/UIElement.h"

namespace engine
{
	class UIButton : public UIElement
	{
		REGISTER_COMPONENT(UIButton);
	private:

	public:
		void DrawUI() const override;

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};
}

