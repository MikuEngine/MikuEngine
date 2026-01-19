#include "EnginePCH.h"
#include "UIButton.h"

#include "Framework/Object/GameObject/GameObject.h"

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
		m_spriteNormal = normal;
		m_spriteHovered = hover;
		m_spritePressed = pressed;
		m_spriteDisabled = disabled;

		ApplyVisual();
	}

	void UIButton::SetInteractable(bool v)
	{
		if (v)
		{
			if (m_state == State::Disabled)
				m_state = State::Normal;
		}
		else
		{
			m_state = State::Disabled;
		}

		ApplyVisual();
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

	void UIButton::OnMouseUp(const Vector2&, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;

		m_state = State::Hovered;
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
		UIElement::Initialize();

		if (!m_background)
		{
			if (auto* go = GetGameObject())
			{
				m_background = go->GetComponent<UIImage>();
				if (!m_background)
					m_background = go->AddComponent<UIImage>();
			}
		}

		if (m_background)
		{
			m_background->m_raycastTarget = false;
		}

		ApplyVisual();
	}

	void UIButton::DrawUI() const
	{
		// UIImage
	}

	void UIButton::ApplyVisual()
	{
		if (!m_background) return;

		std::string sprite;
		Vector4 tint(1, 1, 1, 1);

		switch (m_state)
		{
		case State::Normal:   sprite = m_spriteNormal;   tint = m_tintNormal; break;
		case State::Hovered:  sprite = m_spriteHovered.empty() ? m_spriteNormal : m_spriteHovered;
			tint = m_tintHover; break;
		case State::Pressed:  sprite = m_spritePressed.empty() ? m_spriteNormal : m_spritePressed;
			tint = m_tintPressed; break;
		case State::Disabled: sprite = m_spriteDisabled.empty() ? m_spriteNormal : m_spriteDisabled;
			tint = m_tintDisabled; break;
		}

		if (!sprite.empty() && sprite != "None")
			m_background->SetTexture(sprite);

		m_background->SetColor(tint);
	}

	void UIButton::OnGui()
	{
		std::string selectedTex[4] = {};
		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga" };
		static std::string hlslExtension{ ".hlsl" };

		bool changed = false;

		if (DrawFileSelector("Normal Sprite", "Resource/Texture/UI/Button", texExtensions, selectedTex[0]))
		{
			m_spriteNormal = selectedTex[0];
			changed = true;
		}
		ImGui::SameLine();
		ImGui::Text("Texture: %s", std::filesystem::path(m_spriteNormal).filename().string().c_str());

		if (DrawFileSelector("Hovered Sprite", "Resource/Texture/UI/Button", texExtensions, selectedTex[1]))
		{
			m_spriteHovered = selectedTex[1];
			changed = true;
		}
		ImGui::SameLine();
		ImGui::Text("Texture: %s", std::filesystem::path(m_spriteHovered).filename().string().c_str());

		if (DrawFileSelector("Pressed Sprite", "Resource/Texture/UI/Button", texExtensions, selectedTex[2]))
		{
			m_spritePressed = selectedTex[2];
			changed = true;
		}
		ImGui::SameLine();
		ImGui::Text("Texture: %s", std::filesystem::path(m_spritePressed).filename().string().c_str());

		if (DrawFileSelector("Disabled Sprite", "Resource/Texture/UI/Button", texExtensions, selectedTex[3]))
		{
			m_spriteDisabled = selectedTex[3];
			changed = true;
		}
		ImGui::SameLine();
		ImGui::Text("Texture: %s", std::filesystem::path(m_spriteDisabled).filename().string().c_str());

		if (changed)
			ApplyVisual();

		ImGui::Spacing();
	}

	void UIButton::Save(json& j) const
	{
		UIElement::Save(j);

		j["State"] = (int)m_state;

		j["SpriteNormal"] = m_spriteNormal;
		j["SpriteHover"] = m_spriteHovered;
		j["SpritePressed"] = m_spritePressed;
		j["SpriteDisabled"] = m_spriteDisabled;

		j["TintNormal"] = m_tintNormal;
		j["TintHover"] = m_tintHover;
		j["TintPressed"] = m_tintPressed;
		j["TintDisabled"] = m_tintDisabled;
	}

	void UIButton::Load(const json& j)
	{
		UIElement::Load(j);

		int s = 0;
		JsonGet(j, "State", s);
		m_state = (State)s;

		JsonGet(j, "SpriteNormal", m_spriteNormal);
		JsonGet(j, "SpriteHover", m_spriteHovered);
		JsonGet(j, "SpritePressed", m_spritePressed);
		JsonGet(j, "SpriteDisabled", m_spriteDisabled);

		JsonGet(j, "TintNormal", m_tintNormal);
		JsonGet(j, "TintHover", m_tintHover);
		JsonGet(j, "TintPressed", m_tintPressed);
		JsonGet(j, "TintDisabled", m_tintDisabled);

		ApplyVisual();
	}

	std::string UIButton::GetType() const
	{
		return "UIButton";
	}
}