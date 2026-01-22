#include "EnginePCH.h"
#include "UIProgressBar.h"

#include "Framework/Scene/Scene.h"
#include "Framework/Scene/SceneManager.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UIImage.h"

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

	void UIProgressBar::Initialize()
	{
		UIElement::Initialize();
		CreateVisuals();
		UpdateVisuals();
		m_dirty = true;
	}

	void UIProgressBar::Update()
	{
		UIElement::Update();

		RefreshVisuals();

		if (m_dirty)
		{
			UpdateVisuals();
			m_dirty = false;
		}
	}

	void UIProgressBar::DrawUI() const
	{
		//
	}

	float UIProgressBar::GetValue() const
	{
		return m_value;
	}

	void UIProgressBar::SetValue(float v)
	{
		v = Clamp01(v);

		if (std::fabs(v - m_value) < 1e-6f) return;

		m_value = v;
		m_dirty = true;
	}

	void UIProgressBar::SetDirection(Direction dir)
	{
		if (m_direction == dir) return;
		m_direction = dir;
		m_dirty = true;
	}

	void UIProgressBar::SetFillMode(FillMode mode)
	{
		if (m_fillMode == mode) return;
		m_fillMode = mode;
		m_dirty = true;
	}

	void UIProgressBar::SetSprites(const std::string& background, const std::string& fill)
	{
		m_bgSprite = background;
		m_fillSprite = fill;

		if (m_background)
		{
			m_background->SetTexture(m_bgSprite);
		}

		if (m_fill)
		{
			m_fill->SetTexture(m_fillSprite);
		}

		m_dirty = true;
	}

	void UIProgressBar::SetColors(const Vector4& bg, const Vector4& fill)
	{
		m_bgColor = bg;
		m_fillColor = fill;

		if (m_background)
		{
			m_background->SetColor(m_bgColor);
		}
		if (m_fill)
		{
			m_fill->SetColor(m_fillColor);
		}

		m_dirty = true;
	}

	void UIProgressBar::CreateVisuals()
	{
		if (m_background && m_fill)
			return;

		GameObject* parent = GetGameObject();
		if (!parent) return;

		auto makeChild = [&](const char* name) -> GameObject*
			{
				if (GameObject* exist = FindChildByName(parent, name))
					return exist;

				// UISlider에서 사용하신 패턴 그대로
				Scene* scene = SceneManager::Get().GetScene();
				if (!scene) return nullptr;

				GameObject* go = scene->CreateGameObject(CreateObjectType::UI);
				if (!go) return nullptr;

				go->SetName(name);
				go->GetTransform()->SetParent(parent->GetTransform());

				// UI 자식은 RectTransform 강제
				if (!go->GetComponent<RectTransform>())
					go->AddComponent<RectTransform>();

				return go;
			};

		// Background
		{
			GameObject* bgGO = makeChild("Background");
			if (bgGO)
			{
				if (!bgGO->GetComponent<UIImage>())
					bgGO->AddComponent<UIImage>();

				m_background = bgGO->GetComponent<UIImage>();

				RectTransform* rt = bgGO->GetComponent<RectTransform>();
				rt->SetAnchorMin({ 0.0f, 0.0f });
				rt->SetAnchorMax({ 1.0f, 1.0f });
				rt->SetPivot({ 0.5f, 0.5f });
				rt->SetAnchoredPosition({ 0.0f, 0.0f });
				rt->SetSize(0.0f, 0.0f);
			}
		}

		// Fill
		{
			GameObject* fillGO = makeChild("Fill");
			if (fillGO)
			{
				if (!fillGO->GetComponent<UIImage>())
					fillGO->AddComponent<UIImage>();

				m_fill = fillGO->GetComponent<UIImage>();

				RectTransform* rt = fillGO->GetComponent<RectTransform>();
				rt->SetAnchorMin({ 0.0f, 0.0f });
				rt->SetAnchorMax({ 1.0f, 1.0f });
				rt->SetPivot({ 0.5f, 0.5f });
				rt->SetAnchoredPosition({ 0.0f, 0.0f });
				rt->SetSize(0.0f, 0.0f);
			}
		}

		bool horizontal = (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft);
		if (horizontal) GetRectTransform()->SetSize(300, 50);
		else			GetRectTransform()->SetSize(50, 300);
		m_dirty = true;
	}

	void UIProgressBar::UpdateVisuals()
	{
		if (!RefreshVisuals())return;
		if (!m_background || !m_fill) return;

		if (!m_bgSprite.empty())  m_background->SetTexture(m_bgSprite);
		if (!m_fillSprite.empty()) m_fill->SetTexture(m_fillSprite);

		m_background->SetColor(m_bgColor);
		m_fill->SetColor(m_fillColor);

		// Direction에 따라 기본 크기 설정 - TODO : 이거 지워도 됨
		m_barWidth = GetRectTransform()->GetSize().x;
		m_barHeight = GetRectTransform()->GetSize().y;
		if (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft)
			GetRectTransform()->SetSize(m_barWidth, m_barHeight);
		else
			GetRectTransform()->SetSize(m_barWidth, m_barHeight);

		// Fill 처리
		RectTransform* rootRT = GetRectTransform();
		RectTransform* fillRT = m_fill->GetGameObject()->GetComponent<RectTransform>();
		if (!rootRT || !fillRT)
			return;

		// (화면 픽셀 좌표) 루트 Rect 계산
		auto& gd = GraphicsDevice::Get();
		const D3D11_VIEWPORT vp = gd.GetViewport();
		UIRect rootRect{ 0.0f, 0.0f, vp.Width, vp.Height };
		const UIRect barRect = rootRT->GetWorldRectResolved(rootRect);

		const float v = Clamp01(m_value);

		if (m_fillMode == FillMode::AnchorResize)
		{
			m_fill->SetMaskMode(MaskMode::None);

			switch (m_direction)
			{
			case Direction::LeftToRight:
				fillRT->SetPivot({ 0.5f, 0.5f });
				fillRT->SetAnchorMin({ 0.0f, 0.0f });
				fillRT->SetAnchorMax({ v, 1.0f });
				break;

			case Direction::RightToLeft:
				fillRT->SetPivot({ 0.5f, 0.5f });
				fillRT->SetAnchorMin({ 1.0f - v, 0.0f });
				fillRT->SetAnchorMax({ 1.0f, 1.0f });
				break;

			case Direction::TopToBottom:
				fillRT->SetPivot({ 0.5f, 0.5f });
				fillRT->SetAnchorMin({ 0.0f, 0.0f });
				fillRT->SetAnchorMax({ 1.0f, v });
				break;

			case Direction::BottomToTop:
				fillRT->SetPivot({ 0.5f, 0.5f });
				fillRT->SetAnchorMin({ 0.0f, 1.0f - v });
				fillRT->SetAnchorMax({ 1.0f, 1.0f });
				break;
			}

			fillRT->SetAnchoredPosition({ 0.0f, 0.0f });
			fillRT->SetSize(0.0f, 0.0f);
			return;
		}

		if (m_fillMode == FillMode::PixelMask)
		{
			if (v <= 0.0f)
			{
				m_fill->SetMaskMode(MaskMode::None);

				Vector4 c = m_fillColor;
				c.w = 0.0f;
				m_fill->SetColor(c);
				return;
			}
			{
				m_fill->SetColor(m_fillColor);
			}

			fillRT->SetPivot({ 0.5f, 0.5f });
			fillRT->SetAnchorMin({ 0.0f, 0.0f });
			fillRT->SetAnchorMax({ 1.0f, 1.0f });
			fillRT->SetAnchoredPosition({ 0.0f, 0.0f });
			fillRT->SetSize(0.0f, 0.0f);

			float x0 = barRect.x;
			float y0 = barRect.y;
			float x1 = barRect.x + barRect.w;
			float y1 = barRect.y + barRect.h;

			switch (m_direction)
			{
			case Direction::LeftToRight:
				x1 = barRect.x + barRect.w * v;
				break;

			case Direction::RightToLeft:
				x0 = barRect.x + barRect.w * (1.0f - v);
				break;

			case Direction::TopToBottom:
				y1 = barRect.y + barRect.h * v;
				break;

			case Direction::BottomToTop:
				y0 = barRect.y + barRect.h * (1.0f - v);
				break;
			}

			m_fill->SetMaskMode(MaskMode::Rect);
			m_fill->SetClipRect(Vector4(x0, y0, x1, y1));
			return;
		}
	}

	bool UIProgressBar::RefreshVisuals()
	{
		GameObject* parent = GetGameObject();
		if (!parent) return false;

		GameObject* bgGO = FindChildByName(parent, "Background");
		GameObject* fillGO = FindChildByName(parent, "Fill");
		if (!bgGO || !fillGO) return false;

		UIImage* newBg = bgGO->GetComponent<UIImage>();
		UIImage* newFill = fillGO->GetComponent<UIImage>();
		if (!newBg || !newFill) return false;

		const bool bgChanged = (newBg != m_background);
		const bool fillChanged = (newFill != m_fill);

		m_background = newBg;
		m_fill = newFill;

		if (bgChanged)
		{
			if (!m_bgSprite.empty()) m_background->SetTexture(m_bgSprite);
			m_bgSprite = m_background->GetTexturePath();
		}

		if (fillChanged)
		{
			if (!m_fillSprite.empty()) m_fill->SetTexture(m_fillSprite);
			m_fillSprite = m_fill->GetTexturePath();
		}

		if (bgChanged || fillChanged)
			m_dirty = true;

		return true;
	}

	void UIProgressBar::OnGui()
	{
		UIElement::OnGui();

		bool changed = false;

		float v = m_value;
		if (ImGui::SliderFloat("Value", &v, 0.0f, 1.0f))
		{
			SetValue(v);
			changed = true;
		}

		int dir = (int)m_direction;
		const char* dirItems[] = { "LeftToRight", "RightToLeft", "BottomToTop", "TopToBottom" };
		if (ImGui::Combo("Direction", &dir, dirItems, IM_ARRAYSIZE(dirItems)))
		{
			SetDirection((Direction)dir);
			changed = true;
		}

		int fm = (int)m_fillMode;
		const char* fmItems[] = { "PixelMask", "AnchorResize" };
		if (ImGui::Combo("FillMode", &fm, fmItems, IM_ARRAYSIZE(fmItems)))
		{
			SetFillMode((FillMode)fm);
			changed = true;
		}

		if (ImGui::ColorEdit4("BgColor", &m_bgColor.x)) { m_dirty = true; changed = true; }
		if (ImGui::ColorEdit4("FillColor", &m_fillColor.x)) { m_dirty = true; changed = true; }

		if (changed)
		{
			UpdateVisuals();
		}
	}

	void UIProgressBar::Save(json& j) const
	{
		UIElement::Save(j);

		j["Value"] = m_value;
		j["Direction"] = (int)m_direction;
		j["FillMode"] = (int)m_fillMode;

		j["BgSprite"] = m_bgSprite;
		j["FillSprite"] = m_fillSprite;

		j["BgColor"] = m_bgColor;
		j["FillColor"] = m_fillColor;
	}

	void UIProgressBar::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "Value", m_value);
		m_value = Clamp01(m_value);

		int dir = (int)Direction::LeftToRight;
		JsonGet(j, "Direction", dir);
		m_direction = (Direction)dir;

		int fm = (int)FillMode::AnchorResize;
		JsonGet(j, "FillMode", fm);
		m_fillMode = (FillMode)fm;

		JsonGet(j, "BgSprite", m_bgSprite);
		JsonGet(j, "FillSprite", m_fillSprite);

		JsonGet(j, "BgColor", m_bgColor);
		JsonGet(j, "FillColor", m_fillColor);

		CreateVisuals();
		RefreshVisuals();
		m_dirty = true;
		UpdateVisuals();
	}

	std::string UIProgressBar::GetType() const
	{
		return "UIProgressBar";
	}
}