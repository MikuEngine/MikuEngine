#include "EnginePCH.h"
#include "UIScrollView.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UISlider.h"
#include "Framework/Object/Component/UI/UIImage.h"
#include "Framework/Object/Component/UI/UIText.h"

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

		static GameObject* EnsureChildUI(GameObject* parent, const char* name, bool& outCreated)
		{
			outCreated = false;

			if (!parent) return nullptr;

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
		}

		static Vector4 CalcViewportClipRectPx(UIElement* owner, RectTransform* viewportRT)
		{
			if (!owner || !viewportRT) return Vector4(0, 0, 0, 0);

			Canvas* c = owner->GetCanvasInParent();
			if (!c) return Vector4(0, 0, 0, 0);

			const Vector2 ref = c->GetReferenceResolution();
			const UIRect rootRect{ 0.0f, 0.0f, ref.x, ref.y };

			const UIRect vr = viewportRT->GetWorldRectResolved(rootRect);

			const Vector2 scale = c->GetUIScale();
			const Vector2 offset = c->GetUIOffset();

			const float pxX = offset.x + vr.x * scale.x;
			const float pxY = offset.y + vr.y * scale.y;
			const float pxW = vr.w * scale.x;
			const float pxH = vr.h * scale.y;

			// (L, T, R, B)
			return Vector4(pxX, pxY, pxX + pxW, pxY + pxH);
		}

		static void ApplyMaskToContentSubtree(Transform* t, const Vector4& clipPx)
		{
			if (!t) return;

			GameObject* go = t->GetGameObject();
			if (go)
			{
				if (auto* img = go->GetComponent<UIImage>())
				{
					img->SetMaskMode(MaskMode::Rect);
					img->SetClipRect(clipPx);
				}

				if (auto* txt = go->GetComponent<UIText>())
				{
					txt->SetMaskMode(MaskMode::Rect);
					txt->SetClipRect(clipPx);
				}
			}

			for (Transform* ch : t->GetChildren())
				ApplyMaskToContentSubtree(ch, clipPx);
		}
	}

	RectTransform* UIScrollView::FindChildRTByName(const char* name) const
	{
		GameObject* root = GetGameObject();
		if (!root) return nullptr;

		// 루트 직계에서 먼저 찾기
		if (GameObject* child = FindChildByName(root, name))
			return child->GetComponent<RectTransform>();

		// Viewport 직계에서도 찾기
		if (m_viewportRT)
		{
			if (GameObject* vpGO = m_viewportRT->GetGameObject())
			{
				if (GameObject* child = FindChildByName(vpGO, name))
					return child->GetComponent<RectTransform>();
			}
		}

		return nullptr;
	}

	void UIScrollView::Initialize()
	{
		UIElement::Initialize();
		
		GameObject* parent = GetGameObject();
		if (!parent) return;

		// Viewport
		{
			bool created = false;
			GameObject* vgo = EnsureChildUI(parent, m_viewportName.c_str(), created);

			if (vgo)
			{
				RectTransform* rt = vgo->GetComponent<RectTransform>();
				m_viewportRT = rt;

				if (created && rt)
				{
					rt->SetAnchorMin({ 0.0f, 0.0f });
					rt->SetAnchorMax({ 0.0f, 0.0f });
					rt->SetPivot({ 0.0f, 0.0f });
					rt->SetAnchoredPosition({ 0.0f, 0.0f });

					rt->SetWidth(500.0f);
					rt->SetHeight(500.0f);
				}

				// (선택) 배경 이미지: 입력 막지 않게
				if (auto* img = vgo->GetComponent<UIImage>(); !img)
					img = vgo->AddComponent<UIImage>();
				vgo->GetComponent<UIImage>()->m_raycastTarget = false;
			}
		}

		// Content
		{
			bool created = false;
			GameObject* contentParent = parent;

			if (m_viewportRT && m_viewportRT->GetGameObject())
				contentParent = m_viewportRT->GetGameObject();

			GameObject* cgo = EnsureChildUI(contentParent, m_contentName.c_str(), created);
			if (cgo)
			{
				RectTransform* rt = cgo->GetComponent<RectTransform>();

				if (created && rt)
				{
					rt->SetAnchorMin({ 0.0f, 0.0f });
					rt->SetAnchorMax({ 1.0f, 1.0f });
					rt->SetPivot({ 0.5f, 0.5f });
					rt->SetAnchoredPosition({ 0.0f, 0.0f });
					rt->SetSize(0.0f, 500.0f);
				}
			}
		}

		if (m_scrollbarName.empty())
			m_scrollbarName = "Scrollbar";

		// Scrollbar
		{
			bool created = false;
			GameObject* sgo = EnsureChildUI(parent, m_scrollbarName.c_str(), created);
			if (sgo)
			{
				RectTransform* rt = sgo->GetComponent<RectTransform>();

				if (created && rt)
				{
					rt->SetAnchorMin({ 1.0f, 0.0f });
					rt->SetAnchorMax({ 1.0f, 1.0f });
					rt->SetPivot({ 1.0f, 0.5f });
					rt->SetAnchoredPosition({ 0.0f, 0.0f });
					rt->SetWidth(20.0f);   
					rt->SetHeight(0.0f);   
				}

				UISlider* slider = sgo->GetComponent<UISlider>();
				if (!slider) slider = sgo->AddComponent<UISlider>();

				slider->SetDirection(UISlider::Direction::BottomToTop);
				slider->SetValue(0.0f);
				slider->SetUseFill(false);
			}
		}

		if (!m_contentName.empty())
			m_contentRT = FindChildRTByName(m_contentName.c_str());

		if (!m_scrollbarName.empty())
			SetScrollbarByName(m_scrollbarName);

		m_cachedMaxScroll = -1.0f;
		SetScrollY(m_scrollY, true);
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

		if (GameObject* parent = GetGameObject())
		{
			if (GameObject* child = FindChildByName(parent, m_scrollbarName.c_str()))
				m_scrollbar = child->GetComponent<UISlider>();
		}

		if (!m_scrollbar)
		{
			if (GameObject* sbGO = GameObject::Find(m_scrollbarName.c_str()))
				m_scrollbar = sbGO->GetComponent<UISlider>();
		}

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

		const float viewportH = m_viewportRT->GetWorldRect().h;
		const float contentH = m_contentRT->GetWorldRect().h;

		return std::max(0.0f, contentH - viewportH);
	}

	UIRect UIScrollView::GetViewPortWorldRect() const
	{
		return m_viewportRT ? m_viewportRT->GetWorldRect() : UIRect{};
	}

	void UIScrollView::BindScrollbarCallBack()
	{
		if (!m_scrollbar) return;
		
		m_lastScrollbarV = m_scrollbar->GetValue();

		// Slider 값 변화 -> Scroll 이동
		m_scrollbar->SetOnValueChanged([this](float v)
			{
				if (!this) return;
				if (m_syncGuard) return;

				const float maxS = GetMaxScroll();
				const float clamped = std::clamp(v, 0.0f, 1.0f);

				const float dv = clamped - m_lastScrollbarV;
				m_lastScrollbarV = clamped;

				m_syncGuard = true;
				SetScrollY(m_scrollY + dv * maxS * m_scrollbarDragSpeed, false);
				m_syncGuard = false;
			});
	}

	void UIScrollView::ApplyContentPosition()
	{
		if (!m_contentRT) return;

		Vector2 pos = m_contentRT->GetAnchoredPosition();
		pos.y = -m_scrollY;
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

		ImGui::SliderFloat("DragSpeed", &m_scrollbarDragSpeed, 0.1f, 10.0f);

		float y = m_scrollY;
		if (ImGui::SliderFloat("ScrollY", &y, 0.0f, std::max(0.0f, maxS)))
		{
			SetScrollY(y, true);
		}

		if (ImGui::Button("Rebind Content"))
			SetContentByName(m_contentName);

		if (ImGui::Button("Rebind Scrollbar") && !m_scrollbarName.empty())
			SetScrollbarByName(m_scrollbarName);

		if (m_viewportRT)
		{
			Vector4 cr = CalcViewportClipRectPx(this, m_viewportRT);
			ImGui::Text("ClipRect Px LTRB: %.1f %.1f %.1f %.1f", cr.x, cr.y, cr.z, cr.w);
		}
	}

	void UIScrollView::Save(json& j) const
	{
		UIElement::Save(j);

		j["ContentName"] = m_contentName;
		j["ScrollbarName"] = m_scrollbarName;

		j["ScrollY"] = m_scrollY;
		j["DragSpeed"] = m_scrollbarDragSpeed;
	}

	void UIScrollView::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "ContentName", m_contentName);
		JsonGet(j, "ScrollbarName", m_scrollbarName);

		JsonGet(j, "ScrollY", m_scrollY);
		JsonGet(j, "DragSpeed", m_scrollbarDragSpeed);
	}
}