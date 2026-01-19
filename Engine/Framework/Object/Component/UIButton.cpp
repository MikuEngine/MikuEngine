#include "EnginePCH.h"
#include "UIButton.h"

#include "Framework/Object/Component/UIImage.h"
#include "Framework/Object/Component/UIText.h"

namespace engine
{
	void UIButton::SetOnClick(ClickCallback cb)
	{
		m_onClick = std::move(cb);
	}

	void UIButton::SetSprites(const std::string& normal, const std::string& hover, const std::string& pressed, const std::string& disabled)
	{
	}

	void UIButton::OnMouseEnter(const Vector2& mousePos)
	{
		if (m_state == State::Disabled) return;
		if (m_state == State::Pressed) return;
		m_state = State::Hovered;
		ApplyVisual();
	}

	void UIButton::OnMouseExit(const Vector2&)
	{
		if (m_state == State::Disabled) return;
		if (m_state == State::Pressed) return;
		m_state = State::Normal;
		ApplyVisual();
	}

	void UIButton::OnMouseDown(const Vector2&, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;
		m_state = State::Pressed;
		ApplyVisual();
	}

	void UIButton::OnMouseClick(const Vector2&, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;

		if (m_onClick) m_onClick();
	}

	void UIButton::Initialize()
	{

	}

	void UIButton::DrawUI() const
	{

	}

	void UIButton::ApplyVisual()
	{
		std::string sprite;
		Vector4 tint(1, 1, 1, 1);

		switch (m_state)
		{
		case State::Normal:   sprite = m_spriteNormal;   tint = m_tintNormal; break;
		case State::Hovered:  sprite = m_spriteHover.empty() ? m_spriteNormal : m_spriteHover;
			tint = m_tintHover; break;
		case State::Pressed:  sprite = m_spritePressed.empty() ? m_spriteNormal : m_spritePressed;
			tint = m_tintPressed; break;
		case State::Disabled: sprite = m_spriteDisabled.empty() ? m_spriteNormal : m_spriteDisabled;
			tint = m_tintDisabled; break;
		}
	}

	void UIButton::OnGui()
	{
		
	}

	void UIButton::Save(json& j) const
	{

	}

	void UIButton::Load(const json& j)
	{

	}

	std::string UIButton::GetType() const
	{
		return "UIButton";
	}
}