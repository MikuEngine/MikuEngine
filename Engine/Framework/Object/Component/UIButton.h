#pragma once

#include "Framework/Object/Component/UIElement.h"
#include "Framework/Object/Component/UIInteractable.h"

namespace engine
{
	class UIImage;
	class UIText;

	class UIButton : public UIElement, public UIInteractable
	{
		REGISTER_COMPONENT(UIButton)
	public:
		using ClickCallback = std::function<void()>;

	private:
		enum class State
		{
			Normal,
			Hovered,
			Pressed,
			Disabled
		};

		bool m_hovered = false;
		bool m_pressed = false;

	private:
		State m_state = State::Normal;
		ClickCallback m_onClick;

		UIImage* m_background = nullptr;
		UIText* m_text = nullptr;

		// ImagePath
		std::string m_spriteNormal;
		std::string m_spriteHovered;
		std::string m_spritePressed;
		std::string m_spriteDisabled;

		// Tint
		Vector4 m_tintNormal = Vector4(1, 1, 1, 1);
		Vector4 m_tintHover = Vector4(1, 1, 1, 1);
		Vector4 m_tintPressed = Vector4(1, 1, 1, 1);
		Vector4 m_tintDisabled = Vector4(1, 1, 1, 1);

	public:
		void SetOnClick(ClickCallback cb);
		void SetSprites(const std::string& normal,
						const std::string& hover,
						const std::string& pressed,
						const std::string& disabled);
		
		void SetInteractable(bool v);
		State GetState() const { return m_state; }

	public:
		// Input
		void OnMouseEnter(const Vector2& mousePos) override;
		void OnMouseExit(const Vector2&) override;
		void OnMouseUp(const Vector2&, int mouseButton) override;
		void OnMouseDown(const Vector2&, int mouseButton) override;
		void OnMouseClick(const Vector2&, int mouseButton) override;
		void OnMouseOver(const Vector2&) override;
		void OnMouseCancel(const Vector2& mousePos, int mouseButton) override;

	public:
		// Render
		void Initialize() override;
		void DrawUI() const override;

	private:
		void ApplyVisual();

	public:
		// Component
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};
}