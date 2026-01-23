#pragma once

#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/Object/Component/UI/UIInteractable.h"

namespace engine
{
	class UIImage;
	class UIText;

	class UIButton : public UIElement, public UIInteractable
	{
		REGISTER_COMPONENT(UIButton, UIElement)

	public:
		using ClickCallback = std::function<void()>;
		using HoverCallback = std::function<void(bool)>;

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
		std::vector<ClickCallback> m_onClick;
		std::vector<HoverCallback> m_onHover;

		UIImage* m_background = nullptr;
		UIText* m_label = nullptr;

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

		std::string m_labelText = "Button";

	public:
		// Callback
		void AddOnClick(ClickCallback&& cb);
		void AddOnHover(HoverCallback&& cb);

	public:
		void SetSprites(const std::string& normal,
						const std::string& hover,
						const std::string& pressed,
						const std::string& disabled);
		
		void SetInteractable(bool v);
		State GetState() const { return m_state; }

		UIImage* GetTargetGraphic() const { return m_background; }

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
		void CreateVisuals();
		void UpdateVisuals();

	public:
		// Component
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
	};
}