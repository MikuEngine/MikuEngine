#include "EnginePCH.h"
#include "UIScrollView.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UISlider.h"

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

	RectTransform* UIScrollView::FindChildRTByName(const char* name) const
	{
		GameObject* parent = GetGameObject();
		if (!parent) return nullptr;

		GameObject* child = FindChildByName(parent, name);
		if (!child) return nullptr;

		return child->GetComponent<RectTransform>();
	}

	void UIScrollView::Initialize()
	{
		UIElement::Initialize();
		
		m_viewportRT = GetRectTransform();

		if (!m_contentName.empty())
			m_contentRT = FindChildRTByName(m_contentName.c_str());

		if (!m_scrollbarName.empty())
			SetScrollbarByName(m_scrollbarName);

		BindScrollbarCallBack();

		m_cachedMaxScroll = -1.0f;
		SetScrollY(m_scrollY, true);
	}

	void UIScrollView::Update()
	{
		UIElement::Update();

		if (!m_contentRT && !m_contentName.empty())
			m_contentRT = FindChildRTByName(m_contentName.c_str());

		if (!m_scrollbar && !m_scrollbarName.empty())
		{
			if (GameObject* sbGO = GameObject::Find(m_scrollbarName.c_str()))
				m_scrollbar = sbGO->GetComponent<UISlider>();

			if (m_scrollbar)
				BindScrollbarCallBack();
		}

		const float maxS = GetMaxScroll();
		if (std::fabs(maxS - m_cachedMaxScroll) > 1e-4f)
		{
			m_cachedMaxScroll = maxS;
			SetScrollY(m_scrollY, true);
		}
	}
	void UIScrollView::DrawUI() const
	{
		//
	}
	void UIScrollView::SetContentByName(const std::string& childName)
	{
		m_contentName = childName;
		m_contentRT = FindChildRTByName(m_contentName.c_str());

		m_cachedMaxScroll = -1.0f;
		SetScrollY(m_scrollY, true);
	}

	void UIScrollView::SetScrollbarByName(const std::string& goName)
	{
		m_scrollbarName = goName;
		m_scrollbar = nullptr;

		if (m_scrollbarName.empty())
			return;

		if (GameObject* sbGO = GameObject::Find(m_scrollbarName.c_str()))
			m_scrollbar = sbGO->GetComponent<UISlider>();

		BindScrollbarCallBack();
		SetScrollY(m_scrollY, true);
	}

	void UIScrollView::SetScrollY(float y, bool syncScrollbar)
	{
		const float maxS = GetMaxScroll();
		m_scrollY = std::clamp(y, 0.0f, maxS);

		ApplyContentPosition();

		if (syncScrollbar)
			SyncScrollbarFromScroll();

		EmitScrollChanged();
	}

	float UIScrollView::GetMaxScroll() const
	{
		if (!m_viewportRT || !m_contentRT) return 0.0f;

		const float viewportH = m_viewportRT->GetHeight();
		const float contentH = m_contentRT->GetHeight();

		return std::max(0.0f, contentH - viewportH);
	}

	void UIScrollView::OnBeginDrag(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		m_dragging = true;

		for (auto& cb : m_onBeginDrag)
			if (cb) cb();
	}

	void UIScrollView::OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton)
	{
		if (mouseButton != 0) return;
		if (!m_dragging) return;

		const float newY = m_scrollY - delta.y * m_dragSpeed;
		SetScrollY(newY, true);
	}

	void UIScrollView::OnEndDrag(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;
		if (!m_dragging) return;

		m_dragging = false;

		for (auto& cb : m_onEndDrag)
			if (cb) cb();
	}

	void UIScrollView::OnMouseCancel(const Vector2& mousePos, int mouseButton)
	{
		if (mouseButton != 0) return;

		if (m_dragging)
		{
			m_dragging = false;

			for (auto& cb : m_onEndDrag)
				if (cb) cb();
		}
	}

	void UIScrollView::BindScrollbarCallBack()
	{
		if (!m_scrollbar) return;

		// Slider 값 변화 -> Scroll 이동
		m_scrollbar->SetOnValueChanged([this](float v)
			{
				if (!this) return;
				if (m_syncGuard) return;

				const float maxS = GetMaxScroll();
				const float clamped = std::clamp(v, 0.0f, 1.0f);

				m_syncGuard = true;
				SetScrollY(clamped * maxS, false); // false: Scroll -> Slider 재동기화는 루프라서 막음
				m_syncGuard = false;
			});
	}

	void UIScrollView::ApplyContentPosition()
	{
		if (!m_contentRT) return;

		Vector2 pos = m_contentRT->GetAnchoredPosition();
		pos.y = m_scrollY; // 방향이 반대면 pos.y = -m_scrollY
		m_contentRT->SetAnchoredPosition(pos);
	}

	void UIScrollView::SyncScrollbarFromScroll()
	{
		if (!m_scrollbar) return;
		if (m_syncGuard) return;

		const float maxS = GetMaxScroll();
		const float t = (maxS > 0.0f) ? (m_scrollY / maxS) : 0.0f;

		m_syncGuard = true;
		m_scrollbar->SetValue(std::clamp(t, 0.0f, 1.0f), false);
		m_syncGuard = false;
	}

	void UIScrollView::EmitScrollChanged()
	{
		ScrollInfo info{};
		info.scrollY = m_scrollY;
		info.maxScroll = GetMaxScroll();
		info.normalized = (info.maxScroll > 0.0f) ? (m_scrollY / info.maxScroll) : 0.0f;

		for (auto& cb : m_onScrollChanged)
			if (cb) cb(info);
	}

	void UIScrollView::OnGui()
	{
		UIElement::OnGui();

		float maxS = GetMaxScroll();
		ImGui::Text("ScrollY: %.2f / Max: %.2f", m_scrollY, maxS);

		ImGui::SliderFloat("DragSpeed", &m_dragSpeed, 0.1f, 10.0f);

		float y = m_scrollY;
		if (ImGui::SliderFloat("ScrollY", &y, 0.0f, std::max(0.0f, maxS)))
		{
			SetScrollY(y, true);
		}

		if (ImGui::Button("Rebind Content"))
			SetContentByName(m_contentName);

		if (ImGui::Button("Rebind Scrollbar") && !m_scrollbarName.empty())
			SetScrollbarByName(m_scrollbarName);
	}

	void UIScrollView::Save(json& j) const
	{
		UIElement::Save(j);

		j["ContentName"] = m_contentName;
		j["ScrollbarName"] = m_scrollbarName;

		j["ScrollY"] = m_scrollY;
		j["DragSpeed"] = m_dragSpeed;
	}

	void UIScrollView::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "ContentName", m_contentName);
		JsonGet(j, "ScrollbarName", m_scrollbarName);

		JsonGet(j, "ScrollY", m_scrollY);
		JsonGet(j, "DragSpeed", m_dragSpeed);
	}
}