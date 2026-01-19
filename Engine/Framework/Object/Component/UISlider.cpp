#include "EnginePCH.h"
#include "UISlider.h"

#include "Framework/Object/Component/UIImage.h"

namespace engine
{
	void UISlider::Initialize()
	{

	}

	void UISlider::SetOnValueChanged(ValueChangedCallback cb)
	{
		m_onValueChanged = std::move(cb);
	}

	void UISlider::SetRange(float minV, float maxV)
	{
		m_minValue = std::max(minV, 0.0f);
	}

	bool UISlider::IsInteractable() const
	{
		return m_interactable;
	}

	void UISlider::OnMouseDown(const Vector2& mousePos, int mouseButton)
	{

	}

	void UISlider::OnMouseUp(const Vector2& mousePos, int mouseButton)
	{

	}

	void UISlider::OnBeginDrag(const Vector2& mousePos, int mouseButton)
	{

	}

	void UISlider::OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton)
	{

	}

	void UISlider::OnEndDrag(const Vector2& mousePos, int mouseButton)
	{

	}

	void UISlider::OnGui()
	{

	}

	void UISlider::Save(json& j) const
	{

	}

	void UISlider::Load(const json& j)
	{

	}

	std::string UISlider::GetType() const
	{
		return "UISlider";
	}

	void UISlider::ApplyVisual()
	{

	}
}