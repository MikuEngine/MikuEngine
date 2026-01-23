#include "GamePCH.h"
#include "AimPointer.h"

#include <Core/Graphics/Device/GraphicsDevice.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Canvas.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/Renderer/SpriteRenderer.h>
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
        // 카메라 찾기
        engine::Camera* camera = nullptr;
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* camGO = scene->FindGameObject("MainCamera"))
            {
                camera = camGO->GetComponent<engine::Camera>();
            }
        }

        if (!camera)
        {
            m_worldPosition = engine::Vector3::Zero;
            return;
        }

        // 뷰포트 크기
        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        float screenWidth = (vp.Width > 0.0f) ? vp.Width : 1920.0f;
        float screenHeight = (vp.Height > 0.0f) ? vp.Height : 1080.0f;

        // 화면 좌표를 NDC로 변환 (-1 ~ 1)
        float ndcX = (2.0f * mousePos.x / screenWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * mousePos.y / screenHeight);

        // 역투영 행렬과 역뷰 행렬 계산
        engine::Matrix invProj = camera->GetProjection().Invert();
        engine::Matrix invView = camera->GetView().Invert();

        // NDC에서 뷰 공간으로 변환 (near plane과 far plane 포인트)
        engine::Vector4 nearPointNDC(ndcX, ndcY, 0.0f, 1.0f);
        engine::Vector4 farPointNDC(ndcX, ndcY, 1.0f, 1.0f);

        // 뷰 공간으로 변환
        engine::Vector4 nearPointView = engine::Vector4::Transform(nearPointNDC, invProj);
        engine::Vector4 farPointView = engine::Vector4::Transform(farPointNDC, invProj);

        // w로 나누어 동차 좌표 정규화
        if (std::abs(nearPointView.w) > 0.0001f)
        {
            nearPointView /= nearPointView.w;
        }
        if (std::abs(farPointView.w) > 0.0001f)
        {
            farPointView /= farPointView.w;
        }

        // 월드 공간으로 변환
        engine::Vector4 nearPointWorld = engine::Vector4::Transform(nearPointView, invView);
        engine::Vector4 farPointWorld = engine::Vector4::Transform(farPointView, invView);

        // 레이 원점과 방향 계산
        engine::Vector3 rayOrigin(nearPointWorld.x, nearPointWorld.y, nearPointWorld.z);
        engine::Vector3 rayEnd(farPointWorld.x, farPointWorld.y, farPointWorld.z);
        engine::Vector3 rayDir = rayEnd - rayOrigin;
        rayDir.Normalize();

        // Y=0 평면(XZ 바닥면)과의 교점 계산
        // 레이: P = rayOrigin + t * rayDir
        // 평면: Y = 0
        // rayOrigin.y + t * rayDir.y = 0
        // t = -rayOrigin.y / rayDir.y

        if (std::abs(rayDir.y) > 0.0001f)
        {
            float t = -rayOrigin.y / rayDir.y;

            // t > 0 이면 카메라 앞쪽에 교점이 있음
            if (t > 0.0f)
            {
                m_worldPosition = rayOrigin + rayDir * t;
                m_worldPosition.y = 0.0f; // 정확히 Y=0으로 설정
            }
            else
            {
                // 카메라 뒤쪽이면 기본값
                m_worldPosition = engine::Vector3::Zero;
            }
        }
        else
        {
            // 레이가 Y=0 평면과 평행한 경우 (거의 발생하지 않음)
            m_worldPosition = engine::Vector3::Zero;
        }
    }

    engine::Vector3 AimPointer::GetDirectionFrom(const engine::Vector3& fromPosition) const
    {
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.y = 0.0f;  // XZ 평면에서의 방향 (Y축 무시)
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


