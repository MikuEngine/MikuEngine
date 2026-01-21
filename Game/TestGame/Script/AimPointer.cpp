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

namespace game
{
    void AimPointer::Start()
    {
        /*
        LOG_PRINT("[AimPointer] Started");
        */
        LOG_PRINT("[AimPointer] Started");

        EnsureUICursor();
        SetCursorTexture(m_useAlternateCursor);
        UpdateWorldPositionFromMouse(engine::Input::GetMousePosition());
    }

    void AimPointer::Update()
    {
        /*
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* camPos = scene->FindGameObject("MainCamera"))
            {
                // 현재는 카메라 앞 0.2에 배치하는 형태로 스크린에 붙어보이는 효과를 줌
                // 이후 정식 2D UI렌더러를 이용해 수정예정
                m_worldPosition = camPos->GetTransform()->GetWorldPosition();
                m_worldPosition.z += 0.2f;
            }
        }

        // 마우스 스크린 좌표 가져오기
        engine::Vector2 mousePos = engine::Input::GetMousePosition();
        LOG_PRINT("{} , {}", mousePos.x, mousePos.y);
        //// 탑다운 뷰: 스크린 좌표를 월드 좌표로 변환 (단순화된 버전)
        //// 할 일: 카메라 뷰/프로젝션 행렬을 사용한 정확한 변환 필요
        //// 현재는 대략적인 스케일로 변환 (테스트용)
        //
        //// 화면 중앙을 (0, 0)으로, 스케일 조정
        float screenWidth = 1920;   // 임시 값
        float screenHeight = 1080.0f;   // 임시 값
        float worldScaleX = 0.02f;      // 스크린 -> 월드 변환 스케일 (조정 필요)
        float worldScaleY = 0.02f; 


        //커서 렌더러 오브젝트의 z좌표가 -3 고정일 경우
        //m_worldPosition.z = -3;
        //worldScaleX = 32.7f / screenWidth;
        //worldScaleY = 18.4f / screenHeight;
         
        //커서 렌더러 오브젝트의 z좌표가, 카메라 z좌표 + 0.2 를 추종할 경우 
        worldScaleX = 0.32f / screenWidth;
        worldScaleY = 0.18f / screenHeight;

        m_worldPosition.x += (mousePos.x - screenWidth * 0.5f) * worldScaleX;
        m_worldPosition.y += -(mousePos.y - screenHeight * 0.5f) * worldScaleY;          
        
       
        // 에임포인터 오브젝트 위치 업데이트                
        GetTransform()->SetLocalPosition(m_worldPosition);
        */

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
        engine::Vector3 basePos{ 0.0f, 0.0f, 0.0f };

        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* camPos = scene->FindGameObject("MainCamera"))
            {
                basePos = camPos->GetTransform()->GetWorldPosition();
                basePos.z += 0.2f;
            }
        }

        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        float screenWidth = (vp.Width > 0.0f) ? vp.Width : 1920.0f;
        float screenHeight = (vp.Height > 0.0f) ? vp.Height : 1080.0f;

        float worldScaleX = 0.32f / screenWidth;
        float worldScaleY = 0.18f / screenHeight;

        m_worldPosition = basePos;
        m_worldPosition.x += (mousePos.x - screenWidth * 0.5f) * worldScaleX;
        m_worldPosition.y += -(mousePos.y - screenHeight * 0.5f) * worldScaleY;
    }

    engine::Vector3 AimPointer::GetDirectionFrom(const engine::Vector3& fromPosition) const
    {
        /*
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.z = 0.0f;  // 2D 평면에서의 방향
        //direction.z = 19.8f;
        direction.Normalize();
        return direction;
        */
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.z = 0.0f;  // 2D 평면에서의 방향
        direction.Normalize();
        return direction;
    }

    void AimPointer::OnGui()
    {
        /*
        ImGui::Text("World Position: (%.2f, %.2f, %.2f)", 
            m_worldPosition.x, m_worldPosition.y, m_worldPosition.z);
        */
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
        /*
        Object::Save(j);
        */
        Object::Save(j);
        j["CursorTexturePrimary"] = m_cursorTexturePrimary;
        j["CursorTextureAlternate"] = m_cursorTextureAlternate;
        j["UseAlternateCursor"] = m_useAlternateCursor;
        j["CursorSize"] = m_cursorSize;
        j["CursorPivot"] = m_cursorPivot;
    }

    void AimPointer::Load(const engine::json& j)
    {
        /*
        Object::Load(j);
        */
        Object::Load(j);
        engine::JsonGet(j, "CursorTexturePrimary", m_cursorTexturePrimary);
        engine::JsonGet(j, "CursorTextureAlternate", m_cursorTextureAlternate);
        engine::JsonGet(j, "UseAlternateCursor", m_useAlternateCursor);
        engine::JsonGet(j, "CursorSize", m_cursorSize);
        engine::JsonGet(j, "CursorPivot", m_cursorPivot);

        EnsureUICursor();
        SetCursorTexture(m_useAlternateCursor);
    }

    std::string AimPointer::GetType() const
    {
        /*
        return "AimPointer";
        */
        return "AimPointer";
    }
}


