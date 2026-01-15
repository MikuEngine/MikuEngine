#include "EnginePCH.h"
#include "UIElement.h"
#include <Framework/Object/Component/RectTransform.h>

namespace engine
{
	RectTransform* UIElement::GetRectTransform() const
	{
		return nullptr;
	}
	void UIElement::OnGui()
	{
	}
	void UIElement::Save(json& j) const
	{
	}
	void UIElement::Load(const json& j)
	{
	}
	std::string UIElement::GetType() const
	{
		return std::string();
	}
}