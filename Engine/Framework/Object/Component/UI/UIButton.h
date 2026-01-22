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

		enum class ButtonAction
		{
			// 메인
			StartGame,		// 게임 시작
			OpenOption,		// 설정
			OpenCredit,		// 크레딧
			QuitGame,		// 게임 종료

			// 메인->스타트게임
			EnterPlay,		// 심연 진입
			OpenUpgrade,	// 강화
			//OpenOption,	// 설정
			BackToMain,		// 메인화면으로

			BackToSelect,	// 위 버튼 고르는 곳으로

			None,			// 상태 없음.
		};

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

		ButtonAction m_action = ButtonAction::None;
		std::string m_actionParam = "";

	public:
		void SetOnClick(ClickCallback cb);
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