#include "GamePCH.h"
#include "AimPointer.h"

#include <Core/Graphics/Device/GraphicsDevice.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Canvas.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UIImage.h>
#include <Framework/Object/Component/SpriteRenderer.h>
#include <Framework/Object/Component/Camera.h>

namespace game
{
    void AimPointer::Start()
    {       
        LOG_PRINT("[AimPointer] Started");

        EnsureUICursor();
        SetCursorTexture(m_useAlternateCursor);
        UpdateWorldPositionFromMouse(engine::Input::GetMousePosition());
    }

    void AimPointer::Update()
    {
        

        EnsureUICursor();

        // 마우스 UI 좌표 -> UI 이미지 위치
        engine::Vector2 mousePos = engine::Input::GetMousePosition();
        if (m_cursorRect)
        {
            m_cursorRect->SetAnchoredPosition(mousePos);
        }

        // P 키로 커서 이미지 교체
        if (engine::Input::IsKeyPressed(engine::Keys::P))
        {
            SetCursorTexture(!m_useAlternateCursor);
        }

        // 월드 좌표 계산 (기존 로직 개선)
        UpdateWorldPositionFromMouse(mousePos);
    }

    void AimPointer::SetCursorTexture(bool useAlternate)
    {
        m_useAlternateCursor = useAlternate;

        EnsureUICursor();

        if (!m_cursorImage)
            return;

        const std::string& path = m_useAlternateCursor ? m_cursorTextureAlternate : m_cursorTexturePrimary;
        if (!path.empty() && path != "None")
        {
            m_cursorImage->SetTexture(path);
        }
    }

    void AimPointer::EnsureUICursor()
    {
        engine::GameObject* go = GetGameObject();
        if (!go)
            return;

        m_canvas = go->GetComponent<engine::Canvas>();
        FATAL_CHECK(m_canvas != nullptr, "AimPointer 오브젝트에 Canvas가 필요합니다.");

        if (m_cursorImage && m_cursorRect && m_canvas)
        {
            return;
        }

        if (!m_cursorImage)
        {
            m_cursorImage = go->GetComponent<engine::UIImage>();
            if (!m_cursorImage)
                m_cursorImage = go->AddComponent<engine::UIImage>();
        }

        if (m_cursorImage)
        {
            m_cursorImage->m_raycastTarget = false;
            m_cursorImage->SetAlphaBlend(true);
        }

        if (!m_cursorRect)
        {
            m_cursorRect = m_cursorImage ? m_cursorImage->GetRectTransform() : nullptr;
            if (!m_cursorRect)
            {
                m_cursorRect = go->GetComponent<engine::RectTransform>();
                if (!m_cursorRect)
                    m_cursorRect = go->AddComponent<engine::RectTransform>();
            }
        }

        if (m_cursorRect)
        {
            m_cursorRect->SetAnchorMin({ 0.0f, 0.0f });
            m_cursorRect->SetAnchorMax({ 0.0f, 0.0f });
            m_cursorRect->SetPivot(m_cursorPivot);
            m_cursorRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        }

        if (auto* sprite = go->GetComponent<engine::SpriteRenderer>())
        {
            sprite->SetActive(false);
        }
    }

    void AimPointer::UpdateWorldPositionFromMouse(const engine::Vector2& mousePos)
    {
        engine::Vector3 camPos{ 0.0f, 0.0f, -20.0f };
        float fovDegrees = 50.0f;

        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* camGO = scene->FindGameObject("MainCamera"))
            {
                camPos = camGO->GetTransform()->GetWorldPosition();
                
                // Camera 컴포넌트에서 FOV 가져오기
                if (auto* camera = camGO->GetComponent<engine::Camera>())
                {
                    // Camera의 m_fov는 private이므로, 기본값 사용 또는 public getter 필요
                    // 현재는 씬에 저장된 FOV 값(50.0)을 사용
                    fovDegrees = 50.0f;
                }
            }
        }

        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        float screenWidth = (vp.Width > 0.0f) ? vp.Width : 1920.0f;
        float screenHeight = (vp.Height > 0.0f) ? vp.Height : 1080.0f;

        // 카메라에서 Z=0 평면(게임 월드)까지의 거리
        float distanceToWorld = std::abs(camPos.z);
        
        // FOV와 거리를 기반으로 월드 크기 계산
        // 화면 절반의 세로 월드 크기 = tan(fov/2) * distance
        float fovRadians = DirectX::XMConvertToRadians(fovDegrees);
        float halfWorldHeight = std::tan(fovRadians * 0.5f) * distanceToWorld;
        float halfWorldWidth = halfWorldHeight * (screenWidth / screenHeight);

        // 스크린 좌표 -> 월드 좌표 스케일
        float worldScaleX = (halfWorldWidth * 2.0f) / screenWidth;
        float worldScaleY = (halfWorldHeight * 2.0f) / screenHeight;

        // 카메라 위치를 기준으로 월드 좌표 계산
        m_worldPosition.x = camPos.x + (mousePos.x - screenWidth * 0.5f) * worldScaleX;
        m_worldPosition.y = camPos.y + -(mousePos.y - screenHeight * 0.5f) * worldScaleY;
        m_worldPosition.z = 0.0f;  // 게임 월드는 Z=0 평면
    }

    engine::Vector3 AimPointer::GetDirectionFrom(const engine::Vector3& fromPosition) const
    {      
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.z = 0.0f;  // 2D 평면에서의 방향
        direction.Normalize();
        return direction;
    }

    void AimPointer::OnGui()
    {      
        ImGui::Text("World Position: (%.2f, %.2f, %.2f)",
            m_worldPosition.x, m_worldPosition.y, m_worldPosition.z);

        ImGui::Separator();
        ImGui::Text("UI Cursor");
        ImGui::Text("Press P to swap cursor image.");

        if (ImGui::InputText("Cursor Texture A", &m_cursorTexturePrimary))
        {
            if (!m_useAlternateCursor)
                SetCursorTexture(false);
        }

        if (ImGui::InputText("Cursor Texture B", &m_cursorTextureAlternate))
        {
            if (m_useAlternateCursor)
                SetCursorTexture(true);
        }

        if (ImGui::DragFloat2("Cursor Size", &m_cursorSize.x, 1.0f, 1.0f, 1024.0f))
        {
            if (m_cursorRect)
                m_cursorRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        }

        if (ImGui::DragFloat2("Cursor Pivot", &m_cursorPivot.x, 0.01f, 0.0f, 1.0f))
        {
            if (m_cursorRect)
                m_cursorRect->SetPivot(m_cursorPivot);
        }
    }

    void AimPointer::Save(engine::json& j) const
    {        
        Object::Save(j);
        j["CursorTexturePrimary"] = m_cursorTexturePrimary;
        j["CursorTextureAlternate"] = m_cursorTextureAlternate;
        j["UseAlternateCursor"] = m_useAlternateCursor;
        j["CursorSize"] = m_cursorSize;
        j["CursorPivot"] = m_cursorPivot;
    }

    void AimPointer::Load(const engine::json& j)
    {      
        Object::Load(j);
        engine::JsonGet(j, "CursorTexturePrimary", m_cursorTexturePrimary);
        engine::JsonGet(j, "CursorTextureAlternate", m_cursorTextureAlternate);
        engine::JsonGet(j, "UseAlternateCursor", m_useAlternateCursor);
        engine::JsonGet(j, "CursorSize", m_cursorSize);
        engine::JsonGet(j, "CursorPivot", m_cursorPivot);

        EnsureUICursor();
        SetCursorTexture(m_useAlternateCursor);
    }
}


