#include "GamePCH.h"
#include "PlayerAimDebugTextRenderer.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/AimModeController.h"

#include <Core/Graphics/Device/GraphicsDevice.h>

#include <Framework/Asset/Prefab.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Camera.h>
#include <Framework/Object/Component/Canvas.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIText.h>

namespace game
{
    void PlayerAimDebugTextRenderer::Awake()
    {
        ResolveCanvas();
    }

    void PlayerAimDebugTextRenderer::Start()
    {
        if (auto* camGO = engine::GameObject::Find("MainCamera"))
        {
            m_mainCamera = camGO->GetComponent<engine::Camera>();
        }

        ResolvePlayerRefs();
        EnsureTextObject();
    }

    void PlayerAimDebugTextRenderer::Update()
    {
#ifdef _DEBUG
        if (!m_mainCamera)
        {
            if (auto* camGO = engine::GameObject::Find("MainCamera"))
            {
                m_mainCamera = camGO->GetComponent<engine::Camera>();
            }
        }
        if (!m_mainCamera)
        {
            return;
        }

        ResolveCanvas();
        ResolvePlayerRefs();
        if (!m_player || !m_aim || !EnsureTextObject())
        {
            SetTextVisible(false);
            return;
        }

        engine::GameObject* playerGO = m_player->GetGameObject();
        if (!playerGO || !playerGO->GetTransform())
        {
            SetTextVisible(false);
            return;
        }

        const engine::Vector3 playerPos = playerGO->GetTransform()->GetWorldPosition();

        // line 1: last fired bullet direction on XZ
        char line1[128];
        if (m_player->HasLastFiredDirection())
        {
            engine::Vector3 firedDir = m_player->GetLastFiredDirectionXZ();
            snprintf(line1, sizeof(line1), "Fire XZ Dir : (%.3f, %.3f)", firedDir.x, firedDir.z);
        }
        else
        {
            snprintf(line1, sizeof(line1), "Fire XZ Dir : (none)");
        }

        // line 2: player->cursor direction on XZ
        engine::Vector3 toCursor = m_aim->GetWorldPosition() - playerPos;
        toCursor.y = 0.0f;

        char line2[128];
        if (toCursor.LengthSquared() > 0.0001f)
        {
            toCursor.Normalize();
            snprintf(line2, sizeof(line2), "Aim  XZ Dir : (%.3f, %.3f)", toCursor.x, toCursor.z);
        }
        else
        {
            snprintf(line2, sizeof(line2), "Aim  XZ Dir : (none)");
        }

        std::string text = std::string(line1) + "\n" + line2;
        m_uiText->SetText(text);

        engine::Vector2 screenPos;
        if (!WorldToScreen(playerPos + m_worldOffset, screenPos))
        {
            SetTextVisible(false);
            return;
        }

        const engine::Vector2 finalPos(
            screenPos.x - m_cachedParentRectX,
            screenPos.y - m_cachedParentRectY
        );
        m_textRect->SetAnchoredPosition(finalPos);
        SetTextVisible(true);
#else
        SetTextVisible(false);
#endif
    }

    void PlayerAimDebugTextRenderer::ResolveCanvas()
    {
        if (m_canvas && m_parentRT)
        {
            return;
        }

        engine::GameObject* canvasGO = nullptr;

        // 1) self first
        if (auto* self = GetGameObject())
        {
            if (self->GetComponent<engine::Canvas>())
            {
                canvasGO = self;
            }
        }

        // 2) first canvas in scene
        if (!canvasGO)
        {
            auto* scene = engine::SceneManager::Get().GetScene();
            if (scene)
            {
                for (const auto& go : scene->GetGameObjects())
                {
                    if (go && go->GetComponent<engine::Canvas>())
                    {
                        canvasGO = go.get();
                        break;
                    }
                }
            }
        }

        if (!canvasGO)
        {
            return;
        }

        m_canvas = canvasGO->GetComponent<engine::Canvas>();

        if (auto* rt = canvasGO->GetComponent<engine::RectTransform>())
        {
            engine::RectTransform* parentRT = rt->FindPrentRectTransform();
            m_parentRT = parentRT ? parentRT : rt;
        }
    }

    void PlayerAimDebugTextRenderer::ResolvePlayerRefs()
    {
        if (m_player && m_aim)
        {
            return;
        }

        engine::GameObject* playerGO = engine::GameObject::Find(m_playerObjectName);
        if (!playerGO)
        {
            return;
        }

        m_player = playerGO->GetComponent<PlayerControllerScript>();
        m_aim = playerGO->GetComponent<AimModeController>();
    }

    bool PlayerAimDebugTextRenderer::EnsureTextObject()
    {
        if (m_textObject && m_uiText && m_textRect)
        {
            return true;
        }

        if (!m_canvas)
        {
            return false;
        }

        engine::GameObject* textGO = engine::Prefab::Instantiate(m_prefabName);
        if (!textGO)
        {
            return false;
        }

        // Parent under canvas object for UI rendering.
        if (m_canvas->GetGameObject() && m_canvas->GetGameObject()->GetTransform())
        {
            textGO->GetTransform()->SetParent(m_canvas->GetGameObject()->GetTransform(), false);
        }

        m_uiText = textGO->GetComponent<engine::UIText>();
        m_textRect = textGO->GetComponent<engine::RectTransform>();
        if (!m_uiText || !m_textRect)
        {
            textGO->Destroy();
            m_textObject = nullptr;
            m_uiText = nullptr;
            m_textRect = nullptr;
            return false;
        }

        m_textObject = textGO;
        m_textRect->SetAnchorMin({ 0.0f, 0.0f });
        m_textRect->SetAnchorMax({ 0.0f, 0.0f });
        m_textRect->SetPivot({ 0.5f, 0.5f });
        m_uiText->SetAlignH(engine::UITextAlignH::Center);
        m_uiText->SetFontPixelSize(m_fontSize);

        return true;
    }

    void PlayerAimDebugTextRenderer::SetTextVisible(bool visible)
    {
        if (!m_uiText)
        {
            return;
        }

        engine::Vector4 color = m_uiText->GetColor();
        color.w = visible ? 1.0f : 0.0f;
        m_uiText->SetColor(color);
    }

    bool PlayerAimDebugTextRenderer::WorldToScreen(const engine::Vector3& worldPos, engine::Vector2& screenPos)
    {
        if (!m_mainCamera)
        {
            return false;
        }

        engine::Matrix view = m_mainCamera->GetView();
        engine::Matrix proj = m_mainCamera->GetProjection();

        engine::Vector4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        engine::Vector4 viewPos = engine::Vector4::Transform(pos, view);
        engine::Vector4 clipPos = engine::Vector4::Transform(viewPos, proj);

        if (clipPos.w <= 0.0001f)
        {
            return false;
        }

        const float invW = 1.0f / clipPos.w;
        const float ndcX = clipPos.x * invW;
        const float ndcY = clipPos.y * invW;

        if (m_hideWhenOffscreen &&
            (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f))
        {
            return false;
        }

        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        const float vpW = vp.Width;
        const float vpH = vp.Height;

        if (vpW != m_cachedVpW || vpH != m_cachedVpH)
        {
            m_cachedVpW = vpW;
            m_cachedVpH = vpH;

            if (m_parentRT)
            {
                const engine::UIRect rootRect{ 0.0f, 0.0f, vpW, vpH };
                engine::UIRect parentRect = m_parentRT->GetWorldRectResolved(rootRect);
                m_cachedParentRectX = parentRect.x;
                m_cachedParentRectY = parentRect.y;
            }
            else
            {
                m_cachedParentRectX = 0.0f;
                m_cachedParentRectY = 0.0f;
            }
        }

        screenPos.x = (ndcX * 0.5f + 0.5f) * vpW;
        screenPos.y = (-ndcY * 0.5f + 0.5f) * vpH;
        return true;
    }

    void PlayerAimDebugTextRenderer::OnGui()
    {
        ImGui::InputText("Prefab Name", &m_prefabName);
        ImGui::InputText("Player Object Name", &m_playerObjectName);
        ImGui::DragFloat3("World Offset", &m_worldOffset.x, 0.1f);
        ImGui::Checkbox("Hide When Offscreen", &m_hideWhenOffscreen);
        ImGui::DragInt("Font Size", &m_fontSize, 1, 12, 128);

        if (ImGui::Button("Rebind Player"))
        {
            m_player = nullptr;
            m_aim = nullptr;
            ResolvePlayerRefs();
        }
    }

    void PlayerAimDebugTextRenderer::Save(engine::json& j) const
    {
        Object::Save(j);
        j["PrefabName"] = m_prefabName;
        j["PlayerObjectName"] = m_playerObjectName;
        j["WorldOffset"] = m_worldOffset;
        j["HideWhenOffscreen"] = m_hideWhenOffscreen;
        j["FontSize"] = m_fontSize;
    }

    void PlayerAimDebugTextRenderer::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "PrefabName", m_prefabName);
        engine::JsonGet(j, "PlayerObjectName", m_playerObjectName);
        engine::JsonGet(j, "WorldOffset", m_worldOffset);
        engine::JsonGet(j, "HideWhenOffscreen", m_hideWhenOffscreen);
        engine::JsonGet(j, "FontSize", m_fontSize);
    }
}
