#include "EnginePCH.h"
#include "UISlider.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UIImage.h"

#include "Framework/Object/Component/Canvas.h"

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

		static bool PointInRect(const Vector2& p, const UIRect& r)
		{
			return (p.x >= r.x && p.x <= r.x + r.w &&
				p.y >= r.y && p.y <= r.y + r.h);
		}

		// 핸들 위치는 방향에 따라서 핸들의 절반이 슬라이더의 배경을 넘지 않아야 함.
		static void ComputeHandleRange01(
			const UIRect& barRect,
			const UIRect& handleRect,
			UISlider::Direction dir,
			float& outMin,
			float& outMax)
		{
			outMin = 0.0f;
			outMax = 1.0f;

			const bool horizontal = (dir == UISlider::Direction::LeftToRight || dir == UISlider::Direction::RightToLeft);

			const float barLen = horizontal ? barRect.w : barRect.h;
			const float handleLen = horizontal ? handleRect.w : handleRect.h;

			if (barLen <= 1e-6f || handleLen <= 0.0f)
				return;

			// 핸들이 트랙 밖으로 나가지 않게: 중심이 이동할 수 있는 범위
			const float halfN = (0.5f * handleLen) / barLen; // 정규화(0~1)
			outMin = std::clamp(halfN, 0.0f, 1.0f);
			outMax = std::clamp(1.0f - halfN, 0.0f, 1.0f);

			// 핸들이 트랙보다 큰 경우(또는 거의 같은 경우) 처리
			if (outMax < outMin)
			{
				const float mid = 0.5f;
				outMin = mid;
				outMax = mid;
			}
		}

		// m_value(0~1) -> 실제 핸들 위치 tPos(min~max)
		static float Value01ToPos01(float value01, float min01, float max01)
		{
			value01 = std::clamp(value01, 0.0f, 1.0f);
			if (std::fabs(max01 - min01) < 1e-6f) return min01; // min==max면 고정
			return min01 + (max01 - min01) * value01;
		}

		// 마우스 위치로 얻은 tRaw(0~1 full bar) -> m_value(0~1)
		static float Pos01ToValue01(float pos01, float min01, float max01)
		{
			if (std::fabs(max01 - min01) < 1e-6f) return 0.0f; // 움직일 수 없으면 0으로
			const float v = (pos01 - min01) / (max01 - min01);
			return std::clamp(v, 0.0f, 1.0f);
		}

		static UIRect GetViewportRootRect()
		{
			auto& gd = GraphicsDevice::Get();
			const D3D11_VIEWPORT vp = gd.GetViewport();

			UIRect r;
			r.x = 0.0f;
			r.y = 0.0f;
			r.w = vp.Width;
			r.h = vp.Height;
			return r;
		}

		static Vector2 ScreenToRef(const Vector2& screenPos, const Canvas* c)
		{
			if (!c) return screenPos;

			const Vector2 s = c->GetUIScale();
			const Vector2 o = c->GetUIOffset();

			return {
				(screenPos.x - o.x) / s.x,
				(screenPos.y - o.y) / s.y
			};
		}


	}

	void UISlider::Initialize()
	{
		UIElement::Initialize();

		CreateVisuals();
		RefreshVisuals();

		m_dirty = true;
		UpdateVisuals();
	}

	void UISlider::Update()
	{
		UIElement::Update();
		RefreshVisuals();

		if (m_fillMode == FillMode::PixelMask)
		{
			UpdateVisuals();
			m_dirty = false;
			return;
		}

		if (m_dirty)
		{
			UpdateVisuals();
			m_dirty = false;
		}
	}

	bool UISlider::HitTestPoint(const Vector2& p) const
	{
		const Canvas* c = GetCanvasInParent();

		if (UIElement::HitTestPoint(p))
			return true;

		if (m_handle)
		{
			if (RectTransform* hrt = m_handle->GetRectTransform())
			{
				const UIRect rootRect = GetViewportRootRect();
				const UIRect hr = hrt->GetWorldRectResolved(rootRect);

				if (PointInRect(p, hr))
					return true;
			}
		}

		return false;
	}

	float UISlider::GetValue() const
	{
		return m_value;
	}

	void UISlider::SetValue(float v, bool notify)
	{
		v = Clamp01(v);

		if (std::fabs(v - m_value) < 1e-6f) return;

		m_value = v;
		m_dirty = true;

		UpdateVisuals();

		if (notify && m_onValueChanged)
			m_onValueChanged(m_value);
	}

	void UISlider::SetDirection(Direction dir)
	{
		if (m_direction == dir) return;
		m_direction = dir;
		m_dirty = true;
	}

	void UISlider::SetFillMode(FillMode mode)
	{
		if (m_fillMode == mode) return;
		m_fillMode = mode;
		m_dirty = true;
	}

	void UISlider::SetSprites(const std::string& track, const std::string& fill, const std::string& handle)
	{
		m_bgSprite = track;
		m_fillSprite = fill;
		m_handleSprite = handle;

		m_dirty = true;
		UpdateVisuals();
	}

	void UISlider::SetOnValueChanged(ValueChangedCallback cb)
	{
		m_onValueChanged = std::move(cb);
	}

	void UISlider::CreateVisuals()
	{
		GameObject* parent = GetGameObject();
		if (!parent) return;

		auto makeChild = [&](const char* name) -> GameObject*
			{
				if (GameObject* exist = FindChildByName(parent, name))
					return exist;

				Scene* scene = SceneManager::Get().GetScene();
				if (!scene) return nullptr;

				GameObject* go = scene->CreateGameObject(CreateObjectType::UI);
				if (!go) return nullptr;

				go->SetName(name);
				go->GetTransform()->SetParent(parent->GetTransform());

				if (!go->GetComponent<RectTransform>())
					go->AddComponent<RectTransform>();

				return go;
			};

		// Background
		{
			GameObject* go = makeChild("Background");
			if (go)
			{
				if (!go->GetComponent<UIImage>())
					go->AddComponent<UIImage>();

				m_background = go->GetComponent<UIImage>();

				RectTransform* rt = go->GetComponent<RectTransform>();
				rt->SetAnchorMin({ 0.0f, 0.0f });
				rt->SetAnchorMax({ 1.0f, 1.0f });
				rt->SetPivot({ 0.5f, 0.5f });
				rt->SetAnchoredPosition({ 0.0f, 0.0f });
				rt->SetSize(0.0f, 0.0f);
			}
		}

		// Fill
		if (m_useFill)
		{
			GameObject* go = makeChild("Fill");
			if (go)
			{
				if (!go->GetComponent<UIImage>())
					go->AddComponent<UIImage>();

				m_fill = go->GetComponent<UIImage>();

				RectTransform* rt = go->GetComponent<RectTransform>();
				rt->SetAnchorMin({ 0.0f, 0.0f });
				rt->SetAnchorMax({ 1.0f, 1.0f });
				rt->SetPivot({ 0.5f, 0.5f });
				rt->SetAnchoredPosition({ 0.0f, 0.0f });
				rt->SetSize(0.0f, 0.0f);
			}
		}
		else
		{
			m_fill = nullptr;
		}

		// Handle
		{
			GameObject* hGO = makeChild("Handle");
			if (hGO)
			{
				if (!hGO->GetComponent<UIImage>())
					hGO->AddComponent<UIImage>();

				m_handle = hGO->GetComponent<UIImage>();

				RectTransform* rt = hGO->GetComponent<RectTransform>();
				rt->SetAnchorMin({ 0.5f, 0.5f });
				rt->SetAnchorMax({ 0.5f, 0.5f });
				rt->SetPivot({ 0.5f, 0.5f });
				rt->SetAnchoredPosition({ 0.0f, 0.0f });
			}
		}

		RectTransform* root = GetRectTransform();
		if (root)
		{
			const Vector2 sz = root->GetSize();
			const bool sizeUnset = (sz.x <= 0.0f && sz.y <= 0.0f);

			if (sizeUnset)
			{
				const bool horizontal = (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft);
				if (horizontal) root->SetSize(300.0f, 50.0f);
				else            root->SetSize(50.0f, 300.0f);
			}
		}

		m_dirty = true;
	}

	bool UISlider::RefreshVisuals()
	{
		GameObject* parent = GetGameObject();
		if (!parent) return false;

		GameObject* bgGO = FindChildByName(parent, "Background");
		GameObject* fillGO = FindChildByName(parent, "Fill");
		GameObject* hGO = FindChildByName(parent, "Handle");
		if (!bgGO || !hGO) return false;

		UIImage* newBg = bgGO->GetComponent<UIImage>();
		UIImage* newHandle = hGO->GetComponent<UIImage>();
		if (!newBg || !newHandle) return false;

		UIImage* newFill = nullptr;
		if (m_useFill) // Fill 쓰기로 했는데 없으면 return
		{
			if (!fillGO) return false;
			newFill = fillGO->GetComponent<UIImage>();
			if (!newFill) return false;
		}

		const bool bgChanged = (newBg != m_background);
		const bool fillChanged = (newFill != m_fill);
		const bool handleChanged = (newHandle != m_handle);

		const bool changed = bgChanged || fillChanged || handleChanged;

		m_background = newBg;
		m_fill = newFill;
		m_handle = newHandle;

		if (bgChanged)
		{
			if (!m_bgSprite.empty()) m_background->SetTexture(m_bgSprite);
			if (m_background) m_bgSprite = m_background->GetTexturePath();
		}

		if (fillChanged)
		{
			if (!m_fillSprite.empty()) m_fill->SetTexture(m_fillSprite);
			if (m_fill) m_fillSprite = m_fill->GetTexturePath();
		}

		if (handleChanged)
		{
			if (!m_handleSprite.empty()) m_handle->SetTexture(m_handleSprite);
			if (m_handle) m_handleSprite = m_handle->GetTexturePath();
		}

		if (changed)
			m_dirty = true;

		return true;
	}

	void UISlider::UpdateVisuals()
	{
		if (!RefreshVisuals()) return;

		if (m_background) m_background->m_raycastTarget = false;
		if (m_fill)       m_fill->m_raycastTarget = false;
		if (m_handle)     m_handle->m_raycastTarget = false;

		if (!m_background) return;

		if (!m_bgSprite.empty())  m_background->SetTexture(m_bgSprite);
		m_background->SetColor(m_bgColor);

		if (m_handle)
		{
			if (!m_handleSprite.empty() && m_handleSprite != "None") m_handle->SetTexture(m_handleSprite);
			m_handle->SetColor(m_handleColor);
		}

		if (m_useFill && m_fill)
		{
			if (!m_fillSprite.empty()) m_fill->SetTexture(m_fillSprite);
			m_fill->SetColor(m_fillColor);
		}

		// Fill 처리
		const float v = Clamp01(m_value);
		if (m_useFill && m_fill)
		{
			RectTransform* rootRT = GetRectTransform();
			RectTransform* fillRT = m_fill->GetGameObject()->GetComponent<RectTransform>();
			if (!rootRT || !fillRT) return;

			// (화면 픽셀 좌표) 루트 Rect 계산
			const UIRect rootRect = GetViewportRootRect();
			const UIRect barRect = rootRT->GetWorldRectResolved(rootRect);

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
			}

			if (m_fillMode == FillMode::PixelMask)
			{
				if (v <= 0.0f)
				{
					m_fill->SetMaskMode(MaskMode::None);

					Vector4 c = m_fillColor;
					c.w = 0.0f;
					m_fill->SetColor(c);
				}
				else
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
			}
		}

		// Handle 처리
		if (!m_handle) return;

		RectTransform* handleRT = m_handle->GetGameObject()->GetComponent<RectTransform>();
		if (!handleRT) return;

		Vector2 hs = handleRT->GetSize();
		if (hs.x <= 0.0f && hs.y <= 0.0f)
			handleRT->SetSize(100.0f, 100.0f);

		RectTransform* rootRT = GetRectTransform();
		if (!rootRT) return;

		const UIRect rootRect = GetViewportRootRect();

		const UIRect barRect2 = rootRT->GetWorldRectResolved(rootRect);
		const UIRect handleRect2 = handleRT->GetWorldRectResolved(rootRect);

		float min01 = 0.0f, max01 = 1.0f;
		ComputeHandleRange01(barRect2, handleRect2, m_direction, min01, max01);

		const float tPos = Value01ToPos01(v, min01, max01);

		float ax = 0.5f;
		float ay = 0.5f;

		switch (m_direction)
		{
		case Direction::LeftToRight:
			ax = tPos; ay = 0.5f;
			break;
		case Direction::RightToLeft:
			ax = 1.0f - tPos; ay = 0.5f;
			break;
		case Direction::BottomToTop:
			ax = 0.5f; ay = tPos;
			break;
		case Direction::TopToBottom:
			ax = 0.5f; ay = 1.0f - tPos;
			break;
		}

		handleRT->SetPivot({ 0.5f, 0.5f });
		handleRT->SetAnchorMin({ ax, ay });
		handleRT->SetAnchorMax({ ax, ay });
		handleRT->SetAnchoredPosition({ 0.0f, 0.0f });
	}

	void UISlider::OnMouseDown(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = true;
		m_dragFromHandle = IsMouseOnHandle(mousePos);
		SetValueFromMouse(mousePos);
		UpdateVisuals();
	}

	void UISlider::OnMouseUp(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = false;
		m_dragFromHandle = false;
	}

	void UISlider::OnBeginDrag(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = true;
	}

	void UISlider::OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton)
	{
		if (mouseButton != 0) return;
		if (!m_dragging) return;

		SetValueFromMouse(mousePos);
		UpdateVisuals();
	}

	void UISlider::OnEndDrag(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = false;
		m_dragFromHandle = false;
	}

	void UISlider::OnMouseCancel(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = false;
		m_dragFromHandle = false;
	}

	void UISlider::OnGui()
	{
		UIElement::OnGui();

		bool changed = false;

		if (m_fillMode == FillMode::PixelMask)
		{
			changed = true;
		}

		float v = m_value;
		if (ImGui::SliderFloat("Value", &v, 0.0f, 1.0f))
		{
			SetValue(v, true);
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

		if (ImGui::ColorEdit4("TrackColor", &m_bgColor.x)) { m_dirty = true; changed = true; }

		if (m_useFill)
			if (ImGui::ColorEdit4("FillColor", &m_fillColor.x)) { m_dirty = true; changed = true; }

		if (ImGui::ColorEdit4("HandleColor", &m_handleColor.x)) { m_dirty = true; changed = true; }

		if (changed)
			UpdateVisuals();
	}

	void UISlider::Save(json& j) const
	{
		UIElement::Save(j);

		j["Value"] = m_value;
		j["Direction"] = (int)m_direction;
		j["FillMode"] = (int)m_fillMode;

		j["TrackSprite"] = m_bgSprite;
		j["FillSprite"] = m_fillSprite;
		j["HandleSprite"] = m_handleSprite;

		j["TrackColor"] = m_bgColor;
		j["FillColor"] = m_fillColor;
		j["HandleColor"] = m_handleColor;

		j["UseFill"] = m_useFill;
	}

	void UISlider::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "Value", m_value);

		int dir = (int)Direction::LeftToRight;
		JsonGet(j, "Direction", dir);
		m_direction = (Direction)dir;

		int fm = (int)FillMode::AnchorResize;
		JsonGet(j, "FillMode", fm);
		m_fillMode = (FillMode)fm;

		JsonGet(j, "TrackSprite", m_bgSprite);
		JsonGet(j, "FillSprite", m_fillSprite);
		JsonGet(j, "HandleSprite", m_handleSprite);

		JsonGet(j, "TrackColor", m_bgColor);
		JsonGet(j, "FillColor", m_fillColor);
		JsonGet(j, "HandleColor", m_handleColor);

		JsonGet(j, "UseFill", m_useFill);

		CreateVisuals();
		RefreshVisuals();
		m_dirty = true;
		UpdateVisuals();
	}

	void UISlider::SetValueFromMouse(const Vector2& screenMousePos)
	{
		RectTransform* rt = GetRectTransform();
		if (!rt) return;

		Canvas* c = GetCanvasInParent();
		if (!c) return;

		// screen -> ref
		const Vector2 mousePos = ScreenToRef(screenMousePos, c);

		const UIRect barRect = rt->GetWorldRect(); // ref 좌표

		float min01 = 0.0f, max01 = 1.0f;

		if (m_handle)
		{
			if (RectTransform* hrt = m_handle->GetRectTransform())
			{
				const UIRect handleRect = hrt->GetWorldRect();
				ComputeHandleRange01(barRect, handleRect, m_direction, min01, max01);
			}
		}

		float t = 0.0f;

		switch (m_direction)
		{
		case Direction::LeftToRight:
			t = (barRect.w > 0.0f) ? (mousePos.x - barRect.x) / barRect.w : 0.0f;
			break;
		case Direction::RightToLeft:
			t = (barRect.w > 0.0f) ? 1.0f - (mousePos.x - barRect.x) / barRect.w : 0.0f;
			break;
		case Direction::BottomToTop:
			t = (barRect.h > 0.0f) ? (mousePos.y - barRect.y) / barRect.h : 0.0f;
			break;
		case Direction::TopToBottom:
			t = (barRect.h > 0.0f) ? 1.0f - (mousePos.y - barRect.y) / barRect.h : 0.0f;
			break;
		}

		const float value01 = Pos01ToValue01(Clamp01(t), min01, max01);
		SetValue(value01, true);
	}


	bool UISlider::IsMouseOnHandle(const Vector2& screenMousePos) const
	{
		if (!m_handle) return false;

		Canvas* c = GetCanvasInParent();
		if (!c) return false;

		const Vector2 mousePos = ScreenToRef(screenMousePos, c);

		RectTransform* rt = m_handle->GetRectTransform();
		if (!rt) return false;

		const UIRect r = rt->GetWorldRect();

		return
			mousePos.x >= r.x &&
			mousePos.y >= r.y &&
			mousePos.x <= r.x + r.w &&
			mousePos.y <= r.y + r.h;
	}
}