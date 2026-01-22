#include "EnginePCH.h"
#include "UIButton.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UIImage.h"
#include "Framework/Object/Component/UI/UIText.h"

namespace engine
{
	namespace
	{
		static GameObject* FindChildByName(GameObject* parent, const char* name)
		{
			if (!parent) return nullptr;

			Transform* pt = parent->GetTransform();
			if (!pt) return nullptr;

			for (Transform* ct : pt->GetChildren())
			{
				if (!ct) continue;

				GameObject* child = ct->GetGameObject();
				if (!child) continue;

				if (child->GetName() == name)
					return child;
			}

			return nullptr;
		}
	}

	void UIButton::AddOnClick(ClickCallback&& cb)
	{
		if (!cb) return;
		m_onClick.push_back(std::move(cb));
	}

	void UIButton::SetSprites(const std::string& normal, const std::string& hover, const std::string& pressed, const std::string& disabled)
	{
		m_spriteNormal = normal;
		m_spriteHovered = hover;
		m_spritePressed = pressed;
		m_spriteDisabled = disabled;

		UpdateVisuals();
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

		UpdateVisuals();
	}

	void UIButton::OnMouseEnter(const Vector2& mousePos)
	{
		if (m_state == State::Disabled) return;
		if (m_state == State::Pressed) return;
		m_state = State::Hovered;
		UpdateVisuals();
	}

	void UIButton::OnMouseExit(const Vector2&)
	{
		if (m_state == State::Disabled) return;
		if (m_state == State::Pressed) return;
		m_state = State::Normal;	
		UpdateVisuals();
	}

	void UIButton::OnMouseUp(const Vector2&, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;
	}

	void UIButton::OnMouseDown(const Vector2&, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;
		m_state = State::Pressed;
		UpdateVisuals();
	}

	void UIButton::OnMouseClick(const Vector2&, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;
		m_state = State::Hovered;
		UpdateVisuals();
		
		auto calls = m_onClick;

		for (auto& call : calls)
		{
			if (call) call();
		}
	}

	void UIButton::OnMouseOver(const Vector2&)
	{
		if (m_state == State::Normal)
		{
			m_state = State::Hovered;
			UpdateVisuals();
		}
	}

	void UIButton::OnMouseCancel(const Vector2& mousePos, int mouseButton)
	{
		if (m_state == State::Disabled) return;
		if (mouseButton != 0) return;

		if (m_state != State::Normal)
		{
			m_state = State::Normal;
			UpdateVisuals();
		}
	}

	void UIButton::Initialize()
	{
		UIElement::Initialize();

		CreateVisuals();
		UpdateVisuals();
	}

	void UIButton::DrawUI() const
	{
		// UIImage
	}

	void UIButton::CreateVisuals()
	{
		if (m_label && m_background) return;

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
			m_tintNormal = m_background->GetColor();
		}

		GameObject* parent = GetGameObject();
		if (!parent) return;

		auto makeChild = [&](const char* name, bool& outCreated) -> GameObject*
			{
				outCreated = false;

				if (GameObject* exist = FindChildByName(parent, name))
					return exist;

				Scene* scene = SceneManager::Get().GetScene();
				if (!scene) return nullptr;

				GameObject* go = scene->CreateGameObject(CreateObjectType::UI);
				if (!go) return nullptr;

				outCreated = true;

				go->SetName(name);
				go->GetTransform()->SetParent(parent->GetTransform());

				if (!go->GetComponent<RectTransform>())
					go->AddComponent<RectTransform>();

				return go;
			};

		// Label
		{
			bool created = false;
			GameObject* go = makeChild("Label", created);
			if (go)
			{
				if (!go->GetComponent<UIText>())
					go->AddComponent<UIText>();

				m_label = go->GetComponent<UIText>();

				if (created)
				{
					RectTransform* rt = go->GetComponent<RectTransform>();
					rt->SetAnchorMin({ 0.0f, 0.0f });
					rt->SetAnchorMax({ 1.0f, 1.0f });
					rt->SetPivot({ 0.5f, 0.5f });
					rt->SetAnchoredPosition({ 0.0f, 0.0f });
					rt->SetSize(0.0f, 0.0f);
					m_label->SetText(m_labelText);
					//m_label->SetAlignment(TextAlign::Center);
				}
			}
		}
	}

	void UIButton::UpdateVisuals()
	{
		if (m_label) m_label->m_raycastTarget = false;
	
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
			UpdateVisuals();

		ImGui::Spacing();
	}

	void UIButton::Save(json& j) const
	{
		UIElement::Save(j);

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

		JsonGet(j, "SpriteNormal", m_spriteNormal);
		JsonGet(j, "SpriteHover", m_spriteHovered);
		JsonGet(j, "SpritePressed", m_spritePressed);
		JsonGet(j, "SpriteDisabled", m_spriteDisabled);

		JsonGet(j, "TintNormal", m_tintNormal);
		JsonGet(j, "TintHover", m_tintHover);
		JsonGet(j, "TintPressed", m_tintPressed);
		JsonGet(j, "TintDisabled", m_tintDisabled);

		UpdateVisuals();
	}
}