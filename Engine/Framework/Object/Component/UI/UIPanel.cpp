#include "EnginePCH.h"
#include "UIPanel.h"


#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

#include "Framework/Object/Component/Transform.h"
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

	void UIPanel::Initialize()
	{
		UIElement::Initialize();
		CreateVisuals();
	}

	void UIPanel::CreateVisuals()
	{
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
			m_color = m_background->GetColor();
		}

		m_dirty = true;
	}

	void UIPanel::UpdateVisuals()
	{

	}

	void UIPanel::DrawUI() const
	{
		//
	}

	void UIPanel::Update()
	{
		UpdateVisuals();
	}

	void UIPanel::SetTexture(const std::string& textureFilePath)
	{
		if (textureFilePath.empty() || textureFilePath == "None") return;
		m_bgMode = PanelBackgroundMode::Texture;
		m_bgTexturePath = textureFilePath;
		m_dirty = true;
	}

	const std::string& UIPanel::GetTexturePath() const
	{
		return m_bgTexturePath;
	}

	void UIPanel::SetAlphaBlend(bool enable)
	{
		m_useAlphaBlend = enable;
		m_dirty = true;
	}

	bool UIPanel::IsAlphaBlend() const
	{
		return m_useAlphaBlend;
	}

	void UIPanel::SetColor(const Vector4& color)
	{
		m_color = color;
	}

	const Vector4& UIPanel::GetColor() const
	{
		return m_color;
	}

	bool UIPanel::HasRenderType(RenderType type) const
	{
		return type == RenderType::Screen;
	}

	void UIPanel::Draw(RenderType type) const
	{
		//
	}

	DirectX::BoundingBox UIPanel::GetBounds() const
	{
		return UIElement::GetBounds();
	}

	void UIPanel::OnGui()
	{
		// Only Image
	}

	void UIPanel::Save(json& j) const
	{
		UIElement::Save(j);
	}

	void UIPanel::Load(const json& j)
	{
		UIElement::Load(j);
	}
}