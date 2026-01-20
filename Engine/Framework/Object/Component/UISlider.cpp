#include "EnginePCH.h"
#include "UISlider.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UIImage.h"

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

	void UISlider::Initialize()
	{
		UIElement::Initialize();

		// 기본 이미지
		m_trackSprite = "Resource/Texture/UI/Slider/Track.png";
		m_fillSprite = "Resource/Texture/UI/Slider/Fill.png";
		m_handleSprite = "Resource/Texture/UI/Slider/Handle.png";

		CreateVisuals();
		ApplyVisual();
		UpdateGraphics();
	}

	void UISlider::SetOnValueChanged(ValueChangedCallback cb)
	{
		m_onValueChanged = std::move(cb);
	}

	void UISlider::SetRange(float minV, float maxV)
	{
		if (minV > maxV) std::swap(maxV, minV);

		m_minValue = minV;
		m_maxValue = maxV;

		SetValue(m_value, false);
	}

	void UISlider::SetWholeNumbers(bool v)
	{
		m_wholeNumbers = v;
		SetValue(m_value, false);
	}

	void UISlider::SetValue(float v, bool notify)
	{
		v = Clamp(v, m_minValue, m_maxValue);

		if (m_wholeNumbers)
			v = std::round(v);

		if (std::abs(v - m_value) < 1e-6f) return;

		m_value = v;
		UpdateGraphics();;

		if (notify && m_onValueChanged)
			m_onValueChanged(m_value);
	}

	float UISlider::GetValue() const
	{
		return m_value;
	}

	void UISlider::SetSprites(const std::string& track, const std::string& fill, const std::string& handle)
	{
		m_trackSprite = track;
		m_fillSprite = fill;
		m_handleSprite = handle;

		ApplyVisual();
	}

	bool UISlider::IsInteractable() const
	{
		return m_interactable;
	}

	void UISlider::OnMouseDown(const Vector2& mousePos, int mouseButton)
	{
		if (!m_interactable) return;
		if (mouseButton != 0) return;

		m_dragging = true;
		m_draggingHandle = IsMouseOnHandle(mousePos);

		if (m_draggingHandle && m_handle)
		{
			RectTransform* rtHandle = m_handle->GetRectTransform();
			const UIRect& hr = rtHandle->GetWorldRect();

			const bool horizontal = (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft);

			if (horizontal)
			{
				float handleCenterX = hr.x + hr.w * 0.5f;
				m_handleDragOffset = mousePos.x - handleCenterX;
			}
			else
			{
				float handleCenterY = hr.y + hr.h * 0.5f;
				m_handleDragOffset = mousePos.y - handleCenterY;
			}
		}
		else
		{
			m_handleDragOffset = 0.0f;
			SetValueFromMouse(mousePos, true);
		}
	}

	void UISlider::OnMouseUp(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = false;
		m_draggingHandle = false;
		m_handleDragOffset = 0.0f;
	}

	void UISlider::OnBeginDrag(const Vector2& mousePos, int mouseButton)
	{
		if (!m_interactable) return;
		if (mouseButton != 0) return;
	}

	void UISlider::OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton)
	{
		if (!m_interactable) return;
		if (mouseButton != 0) return;
		if (!m_dragging) return;

		Vector2 p = mousePos;

		if (m_draggingHandle)
		{
			const bool horizontal = (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft);

			if (horizontal) p.x -= m_handleDragOffset;
			else			p.y -= m_handleDragOffset;
		}

		SetValueFromMouse(p, true);
	}

	void UISlider::OnEndDrag(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = false;
		m_draggingHandle = false;
		m_handleDragOffset = 0.0f;
	}

	void UISlider::OnGui()
	{
		ImGui::Checkbox("Interactable", &m_interactable);
		ImGui::Checkbox("Whole Numbers", &m_wholeNumbers);

		ImGui::DragFloat("Min", &m_minValue, 0.1f);
		ImGui::DragFloat("Max", &m_maxValue, 0.1f);

		if (m_maxValue < m_minValue) std::swap(m_maxValue, m_minValue);

		float v = m_value;
		if (ImGui::SliderFloat("Value", &v, m_minValue, m_maxValue))
			SetValue(v, true);

		std::string selectedTex;
		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga" };
		
		bool changed = false;

		if (DrawFileSelector("Track Sprite", "Resource/Texture/UI/Slider", texExtensions, selectedTex))
		{
			m_trackSprite = selectedTex; changed = true;
		}
		ImGui::SameLine(); ImGui::Text("%s", std::filesystem::path(m_trackSprite).filename().string().c_str());

		if (DrawFileSelector("Fill Sprite", "Resource/Texture/UI/Slider", texExtensions, selectedTex))
		{
			m_fillSprite = selectedTex; changed = true;
		}
		ImGui::SameLine(); ImGui::Text("%s", std::filesystem::path(m_fillSprite).filename().string().c_str());

		if (DrawFileSelector("Handle Sprite", "Resource/Texture/UI/Slider", texExtensions, selectedTex))
		{
			m_handleSprite = selectedTex; changed = true;
		}
		ImGui::SameLine(); ImGui::Text("%s", std::filesystem::path(m_handleSprite).filename().string().c_str());

		changed |= ImGui::ColorEdit4("Track Color", &m_trackColor.x);
		changed |= ImGui::ColorEdit4("Fill Color", &m_fillColor.x);
		changed |= ImGui::ColorEdit4("Handle Color", &m_handleColor.x);

		static const char* dirLabels[] =
		{
			"Left To Right",
			"Right To Left",
			"Bottom To Top",
			"Top To Bottom"
		};

		int dir = (int)m_direction;
		if (ImGui::Combo("Direction", &dir, dirLabels, IM_ARRAYSIZE(dirLabels)))
		{
			m_direction = (Direction)dir;
			UpdateGraphics();
		}

		if (changed)
		{
			ApplyVisual();
			UpdateGraphics();
		}
	}

	void UISlider::Save(json& j) const
	{
		UIElement::Save(j);

		j["Interactable"] = m_interactable;
		j["WholeNumbers"] = m_wholeNumbers;
		j["Min"] = m_minValue;
		j["Max"] = m_maxValue;
		j["Value"] = m_value;
		j["Direction"] = (int)m_direction;

		j["TrackSprite"] = m_trackSprite;
		j["FillSprite"] = m_fillSprite;
		j["HandleSprite"] = m_handleSprite;

		j["TrackColor"] = m_trackColor;
		j["FillColor"] = m_fillColor;
		j["HandleColor"] = m_handleColor;

		j["HandlePivot"] = m_handlePivot;
	}

	void UISlider::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "Interactable", m_interactable);
		JsonGet(j, "WholeNumbers", m_wholeNumbers);
		JsonGet(j, "Min", m_minValue);
		JsonGet(j, "Max", m_maxValue);
		JsonGet(j, "Value", m_value);

		int dir = 0;
		JsonGet(j, "Direction", dir);
		m_direction = (Direction)dir;

		JsonGet(j, "TrackSprite", m_trackSprite);
		JsonGet(j, "FillSprite", m_fillSprite);
		JsonGet(j, "HandleSprite", m_handleSprite);

		JsonGet(j, "TrackColor", m_trackColor);
		JsonGet(j, "FillColor", m_fillColor);
		JsonGet(j, "HandleColor", m_handleColor);

		CreateVisuals();

		JsonGet(j, "HandlePivot", m_handlePivot);

		ApplyVisual();
		SetValue(m_value, false);
		UpdateGraphics();
	}

	std::string UISlider::GetType() const
	{
		return "UISlider";
	}

	void UISlider::ApplyVisual()
	{
		if (!m_track || !m_fill || !m_handle) return;

		if (!m_trackSprite.empty() && m_trackSprite != "None")
			m_track->SetTexture(m_trackSprite);
		if (!m_fillSprite.empty() && m_fillSprite != "None")
			m_fill->SetTexture(m_fillSprite);
		if (!m_handleSprite.empty() && m_handleSprite != "None")
			m_handle->SetTexture(m_handleSprite);

		m_track->SetColor(m_trackColor);
		m_fill->SetColor(m_fillColor);
		m_handle->SetColor(m_handleColor);
	}

	void UISlider::CreateVisuals()
	{
		GameObject* parent = GetGameObject();
		if (!parent) return;

		auto getOrCreateChild = [&](const char* name) -> GameObject*
		{
			if (GameObject* found = FindChildByName(parent, name)) return found;
				
			GameObject* go = SceneManager::Get().GetScene()->CreateGameObject(CreateObjectType::UI);
			go->SetName(name);
			go->GetTransform()->SetParent(parent->GetTransform());

			if (!go->GetComponent<RectTransform>())
				go->AddComponent<RectTransform>();

			return go;
		};

		{
			GameObject* go = getOrCreateChild("Background");
			RectTransform* rt = go->GetComponent<RectTransform>();
			rt->SetAnchorMin({0.0f, 0.0f});
			rt->SetAnchorMax({ 1.0f, 1.0f });
			rt->SetPivot({ 0.5f, 0.5f });
			rt->SetAnchoredPosition({ 0.0f, 0.0f });
			rt->SetSize(0.0f, 0.0f);

			m_track = go->GetComponent<UIImage>();
			if (!m_track) m_track = go->AddComponent<UIImage>();
			m_track->m_raycastTarget = false;
			m_track->m_orderInLayer = this->m_orderInLayer + 0;
		}

		{
			GameObject* go = getOrCreateChild("Fill");
			RectTransform* rt = go->GetComponent<RectTransform>();
			rt->SetAnchorMin({ 0.0f, 0.5f });
			rt->SetAnchorMax({ 0.0f, 0.5f });
			rt->SetPivot({ 0.0f, 0.5f });
			rt->SetAnchoredPosition({ 0.0f, 0.0f });
			rt->SetSize(0.0f, 0.0f);

			m_fill = go->GetComponent<UIImage>();
			if (!m_fill) m_fill = go->AddComponent<UIImage>();
			m_fill->m_raycastTarget = false;
			m_fill->m_orderInLayer = this->m_orderInLayer + 1;
		}

		{
			GameObject* go = getOrCreateChild("Handle");
			RectTransform* rt = go->GetComponent<RectTransform>();
			rt->SetAnchorMin({ 0.0f, 0.5f });
			rt->SetAnchorMax({ 0.0f, 0.5f });
			rt->SetPivot({ 0.5f, 0.5f });
			rt->SetAnchoredPosition({ 0.0f, 0.0f });

			m_handle = go->GetComponent<UIImage>();
			if (!m_handle) m_handle =  go->AddComponent<UIImage>();
			m_handle->m_raycastTarget = false;
			m_handle->m_orderInLayer = this->m_orderInLayer + 2;
		}
	}

	void UISlider::UpdateGraphics()
	{
		if (!m_track || !m_fill || !m_handle) return;

		RectTransform* rt = GetRectTransform();
		if (!rt) return;

		const UIRect& r = rt->GetWorldRect();

		float range = (m_maxValue - m_minValue);
		float t = (range <= 0.0f) ? 0.0f : (m_value - m_minValue) / range;

		t = Clamp(t, 0.0f, 1.0f);

		float w = rt->GetWorldRect().w;
		float h = rt->GetWorldRect().h;

		RectTransform* rtHandle = m_handle->GetRectTransform();
		float thumbW = rtHandle->GetSize().x;
		float thumbH = rtHandle->GetSize().y;

		const bool horizontal = (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft);
		const bool reverse = (m_direction == Direction::RightToLeft || m_direction == Direction::TopToBottom);
		
		if (reverse)
		{
			t = 1.0f - t;
		}

		RectTransform* rtFill = m_fill->GetRectTransform();

		if (!rtFill || !rtHandle) return;

		if (horizontal)
		{
			rtFill->SetPivot({ 0.0f, 0.5f });
			rtFill->SetAnchoredPosition({ 0.0f, 0.0f });
			rtFill->SetSize(r.w * t, r.h);

			Vector2 p = rtHandle->GetPivot();

			float minX = thumbW * 0.5f;
			float maxX = r.w - thumbW * 0.5f;
			float x = minX + (maxX - minX) * t;

			//x = r.w * t;
			float xCenter = minX + (maxX - minX) * t;
			float ax = xCenter - (0.5f - p.x) * thumbW;
			float ay = 0.0f - (0.5f - p.y) * thumbH;

			rtHandle->SetPivot(m_handlePivot);
			//rtHandle->SetAnchoredPosition({ x, 0.0f });
			rtHandle->SetAnchoredPosition({ ax, ay });
		}
		else
		{
			rtFill->SetPivot({ 0.5f, 0.0f });
			rtFill->SetAnchoredPosition({ 0.0f, 0.0f });
			rtFill->SetSize(r.w, r.h * t);

			Vector2 p = rtHandle->GetPivot();

			float minY = thumbH * 0.5f;
			float maxY = r.h - thumbH * 0.5f;
			float y = minY + (maxY - minY) * t;

			// y = r.h * t;
			float yCenter = minY + (maxY - minY) * t;
			rtHandle->SetPivot(m_handlePivot);
			//rtHandle->SetAnchoredPosition({ 0.0f, y });

			float ax = 0.0f - (0.5f - p.x) * thumbW;
			float ay = yCenter - (0.5f - p.y) * thumbH;

			rtHandle->SetAnchoredPosition({ ax, ay });
		}
	}

	float UISlider::Clamp(float v, float minV, float maxV)
	{
		return (v < minV) ? minV : (v > maxV ? maxV : v);
	}

	void UISlider::SetValueFromMouse(const Vector2& mousePos, bool notify)
	{
		RectTransform* rt = GetRectTransform();
		if (!rt) return;
		
		const UIRect& r = rt->GetWorldRect();
		if (r.w <= 0.0f || r.h <= 0.0f) return;

		float t = 0.0f;

		const bool horizontal = (m_direction == Direction::LeftToRight || m_direction == Direction::RightToLeft);
		const bool reverse = (m_direction == Direction::RightToLeft || m_direction == Direction::TopToBottom);

		if (horizontal) t = (mousePos.x - r.x) / r.w;
		else            t = (mousePos.y - r.y) / r.h;

		t = Clamp(t, 0.0f, 1.0f);
		if (reverse) t = 1.0f - t;

		float v = m_minValue + (m_maxValue - m_minValue) * t;
		SetValue(v, notify);
	}

	bool UISlider::IsMouseOnHandle(const Vector2& mousePos) const
	{
		if (!m_handle) return false;

		RectTransform* rtHandle = m_handle->GetRectTransform();
		if (!rtHandle) return false;

		const UIRect& hr = rtHandle->GetWorldRect();
		return (mousePos.x >= hr.x && mousePos.x <= hr.x + hr.w &&
			mousePos.y >= hr.y && mousePos.y <= hr.y + hr.h);
	}
}