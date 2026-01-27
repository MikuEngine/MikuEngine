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

		static void CollectRenderersInSubtree(Transform* t,
			std::vector<UIImage*>& outImgs,
			std::vector<UIText*>& outTxts)
		{
			if (!t) return;

			if (GameObject* go = t->GetGameObject())
			{
				if (auto* img = go->GetComponent<UIImage>())
					outImgs.push_back(img);

				if (auto* txt = go->GetComponent<UIText>())
					outTxts.push_back(txt);
			}

			for (Transform* ch : t->GetChildren())
				CollectRenderersInSubtree(ch, outImgs, outTxts);
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
				m_viewportRT = vgo->GetComponent<RectTransform>();

				if (auto* img = vgo->GetComponent<UIImage>(); !img)
					img = vgo->AddComponent<UIImage>();
				vgo->GetComponent<UIImage>()->m_raycastTarget = false;
			}
		}

		// Content
		{
			bool created = false;
			GameObject* contentParent = (m_viewportRT && m_viewportRT->GetGameObject())
				? m_viewportRT->GetGameObject()
				: parent;

			GameObject* cgo = EnsureChildUI(contentParent, m_contentName.c_str(), created);
			if (cgo)
				m_contentRT = cgo->GetComponent<RectTransform>();
		}

		if (m_scrollbarName.empty())
			m_scrollbarName = "Scrollbar";

		// Scrollbar
		{
			bool created = false;
			GameObject* sgo = EnsureChildUI(parent, m_scrollbarName.c_str(), created);
			if (sgo)
			{
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

		// 레이아웃 적용
		ApplyLayout();

		m_cachedMaxScroll = -1.0f;
		SetScrollY(m_scrollY, true);

		m_rendererCacheDirty = true;
		RefreshClipFromViewport(true);
	}

	void UIScrollView::SetContentByName(const std::string& childName)
	{
		m_contentName = childName;
		m_contentRT = FindChildRTByName(m_contentName.c_str());

		m_cachedMaxScroll = -1.0f;
		SetScrollY(m_scrollY, true);

		m_rendererCacheDirty = true;
		RefreshClipFromViewport(true);
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
		SyncScrollbarFromScroll();
		SetScrollY(m_scrollY, true);
	}

	void UIScrollView::SetScrollY(float y, bool syncScrollbar)
	{
		const float maxS = GetMaxScroll();
		m_scrollY = std::clamp(y, 0.0f, maxS);

		ApplyContentPosition();

		if (syncScrollbar)
		{
			SyncScrollbarFromScroll();

			if (m_scrollbar)
				m_lastScrollbarV = m_scrollbar->GetValue();
		}

		// 클립 유지
		RefreshClipFromViewport(false);

		EmitScrollChanged();
	}

	float UIScrollView::GetMaxScroll() const
	{
		if (!m_viewportRT || !m_contentRT) return 0.0f;

		Canvas* c = GetCanvasInParent();
		if (!c) return 0.0f;

		const Vector2 ref = c->GetReferenceResolution();
		const UIRect rootRect{ 0.0f, 0.0f, ref.x, ref.y };

		const float viewportH = m_viewportRT->GetWorldRectResolved(rootRect).h;
		const float contentH = m_contentRT->GetWorldRectResolved(rootRect).h;

		return std::max(0.0f, contentH - viewportH);
	}

	UIRect UIScrollView::GetViewPortWorldRect() const
	{
		return m_viewportRT ? m_viewportRT->GetWorldRect() : UIRect{};
	}

	void UIScrollView::ApplyLayout()
	{
		if (m_viewportRT)
		{
			m_viewportRT->SetAnchorMin({ 0.0f, 0.0f });
			m_viewportRT->SetAnchorMax({ 0.0f, 0.0f });
			m_viewportRT->SetPivot({ 0.0f, 0.0f });
			m_viewportRT->SetAnchoredPosition({ 0.0f, 0.0f });
			m_viewportRT->SetWidth(m_viewportSize);
			m_viewportRT->SetHeight(m_viewportSize);
		}

		if (m_contentRT)
		{
			m_contentRT->SetAnchorMin({ 0.0f, 0.0f });
			m_contentRT->SetAnchorMax({ 1.0f, 1.0f });
			m_contentRT->SetPivot({ 0.5f, 0.5f });
			m_contentRT->SetSize(0.0f, m_contentHeight);
		}

		if (m_scrollbar)
		{
			if (auto* sbRT = m_scrollbar->GetRectTransform())
			{
				sbRT->SetAnchorMin({ 0.0f, 0.0f });
				sbRT->SetAnchorMax({ 0.0f, 0.0f });
				sbRT->SetPivot({ 0.0f, 0.0f });

				const float x = m_viewportSize + m_scrollbarGap;
				sbRT->SetAnchoredPosition({ x, 0.0f });

				sbRT->SetWidth(m_scrollbarWidth);
				sbRT->SetHeight(m_viewportSize);
			}
		}
	}

	void UIScrollView::BindScrollbarCallBack()
	{
		if (!m_scrollbar) return;
		
		m_lastScrollbarV = m_scrollbar->GetValue();

		auto self = engine::Ptr<UIScrollView>(this);

		// Slider 값 변화 -> Scroll 이동
		m_scrollbar->SetOnValueChanged([self](float v)
			{
				if (!self) return;
				UIScrollView* sv = self.Get();

				if (sv->m_syncGuard) return;

				const float maxS = sv->GetMaxScroll();
				const float clamped = std::clamp(v, 0.0f, 1.0f);

				const float dv = clamped - sv->m_lastScrollbarV;
				sv->m_lastScrollbarV = clamped;

				sv->m_syncGuard = true;

				// 스크롤 갱신
				sv->SetScrollY(sv->m_scrollY + dv * maxS * sv->m_scrollbarDragSpeed, false);

				sv->m_syncGuard = false;
			});

		RefreshClipFromViewport(true);
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

		const float clampedT = std::clamp(t, 0.0f, 1.0f);

		m_syncGuard = true;
		m_scrollbar->SetValue(clampedT, false);
		m_scrollbar->ForceUpdateVisuals();
		m_lastScrollbarV = clampedT;

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

	void UIScrollView::RebuildRendererCache()
	{
		if (!m_rendererCacheDirty) return;
		m_rendererCacheDirty = false;

		m_cachedImages.clear();
		m_cachedTexts.clear();

		if (!m_contentRT) return;

		CollectRenderersInSubtree(m_contentRT->GetTransform(), m_cachedImages, m_cachedTexts);
	}

	void UIScrollView::ApplyClipToCachedRenderers(const Vector4& clipPx)
	{
		for (auto* img : m_cachedImages)
		{
			if (!img) continue;
			img->SetMaskMode(MaskMode::Rect);
			img->SetClipRect(clipPx);
		}

		// 텍스트
		for (auto* txt : m_cachedTexts)
		{
			if (!txt) continue;
			txt->SetMaskMode(MaskMode::Rect);
			txt->SetClipRect(clipPx);
		}
	}

	void UIScrollView::RefreshClipFromViewport(bool force)
	{
		if (!m_viewportRT || !m_contentRT) return;

		// 캐시 재구성
		RebuildRendererCache();

		const Vector4 clipPx = CalcViewportClipRectPx(this, m_viewportRT);

		// clip 변화 없으면 스킵
		if (!force)
		{
			const float eps = 0.01f;
			if (std::fabs(clipPx.x - m_cachedClipPx.x) < eps &&
				std::fabs(clipPx.y - m_cachedClipPx.y) < eps &&
				std::fabs(clipPx.z - m_cachedClipPx.z) < eps &&
				std::fabs(clipPx.w - m_cachedClipPx.w) < eps)
			{
				return;
			}
		}

		m_cachedClipPx = clipPx;
		ApplyClipToCachedRenderers(clipPx);
	}

	void UIScrollView::OnScroll(const Vector2& mousePos, float wheelDelta)
	{
		const float wheelStep = 5.0f;
		SetScrollY(m_scrollY - wheelDelta / wheelStep, true);
	}

	bool UIScrollView::HitTestPoint(const Vector2& p) const
	{
		if (!m_viewportRT)
			return UIElement::HitTestPoint(p);

		const Vector4 cr = CalcViewportClipRectPx(const_cast<UIScrollView*>(this), m_viewportRT);

		return (p.x >= cr.x && p.x <= cr.z &&
			p.y >= cr.y && p.y <= cr.w);
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

		ImGui::SliderFloat("ContentHeight", &m_contentHeight, 0.0f, 1000.0f);

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

		j["ViewportSize"] = m_viewportSize;
		j["ScrollbarWidth"] = m_scrollbarWidth;
		j["ScrollbarGap"] = m_scrollbarGap;

		j["ScrollY"] = m_scrollY;
		j["ContentHeight"] = m_contentHeight;
		j["DragSpeed"] = m_scrollbarDragSpeed;
	}

	void UIScrollView::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "ContentName", m_contentName);
		JsonGet(j, "ScrollbarName", m_scrollbarName);

		JsonGet(j, "ViewportSize", m_viewportSize);
		JsonGet(j, "ScrollbarWidth", m_scrollbarWidth);
		JsonGet(j, "ScrollbarGap", m_scrollbarGap);

		JsonGet(j, "ScrollY", m_scrollY);
		JsonGet(j, "ContentHeight", m_contentHeight);
		JsonGet(j, "DragSpeed", m_scrollbarDragSpeed);

		m_contentRT = !m_contentName.empty() ? FindChildRTByName(m_contentName.c_str()) : nullptr;
		if (!m_scrollbarName.empty())
			SetScrollbarByName(m_scrollbarName);

		ApplyLayout();

		m_rendererCacheDirty = true;
		RefreshClipFromViewport(true);
		SetScrollY(m_scrollY, true);
	}
}