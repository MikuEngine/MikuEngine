#include "GamePCH.h"
#include "AimModeController.h"

#include <Core/Graphics/Device/GraphicsDevice.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Canvas.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/Renderer/SpriteRenderer.h>
#include <Framework/Object/Component/Camera.h>

#include <Core/Graphics/Resource/ResourceManager.h>

#include <Scene/GameScene.h>

namespace game
{
    void AimModeController::Awake()
    {
        m_cursorTexByState[(int)AimCursorState::Default] = "Resource/Texture/UI/Cursor/Cursor_Default.png";
        m_cursorTexByState[(int)AimCursorState::Clicked] = "Resource/Texture/UI/Cursor/Cursor_Click.png";

        m_cursorTexByState[(int)AimCursorState::AimIdle] = "Resource/Texture/UI/Cursor/Aim_Idle.png";
        m_cursorTexByState[(int)AimCursorState::AimFiring] = "Resource/Texture/UI/Cursor/Aim_Firing.png";
        m_cursorTexByState[(int)AimCursorState::AimExecute] = "Resource/Texture/UI/Cursor/Aim_Execute.png";

        ////////////////////////////////////////////////////////////////////////////////////////////

        m_cursorPivotByState[(int)AimCursorState::Default] = { 0.0f, 0.0f };
        m_cursorPivotByState[(int)AimCursorState::Clicked] = { 0.0f, 0.0f };

        m_cursorPivotByState[(int)AimCursorState::AimIdle] = { 0.5f, 0.5f };
        m_cursorPivotByState[(int)AimCursorState::AimFiring] = { 0.5f, 0.5f };
        m_cursorPivotByState[(int)AimCursorState::AimExecute] = { 0.5f, 0.5f };
    }

    void AimModeController::Start()
    {       
        LOG_PRINT("[AimPointer] Started");

        std::string sceneName = engine::SceneManager::Get().GetScene()->GetName();

        // 타이틀, 튜토리얼, 로비가 아닐 때만 전투 에임 활성화
        std::string tutorial = GameScene::Name(SceneID::TutorialLobby);
        std::string main = GameScene::Name(SceneID::Main);
        std::string lobby = GameScene::Name(SceneID::Lobby);

        if (sceneName != lobby && sceneName != main && sceneName != tutorial)
        {
            SetCombatAimEnabled(true);
        }
        else
        {
            SetCombatAimEnabled(false);
        }

        EnsureUICursor();

        const engine::Vector2 mousePx = engine::Input::GetMousePosition();
        const AimMode mode = ComputeEffectiveMode();

        m_cursor = AimCursorState::Count; // "무효 값"으로 만들어서 아래에서 반드시 SetCursorTexture 호출되게
        const AimCursorState desired = ComputeDesiredCursorState(mode);
        SetCursorTexture(desired);

        TickWorldAim(mousePx, mode);
        TickUICursor(mousePx, mode);
    }

    void AimModeController::Update()
    {
        // 클라이언트 상의 마우스 위치
        engine::Vector2 mousePx = engine::Input::GetMousePosition();
        const AimMode mode = ComputeEffectiveMode();

        TickWorldAim(mousePx, mode);
        TickUICursor(mousePx, mode);
    }

    void AimModeController::SetCombatAimEnabled(bool enabled)
    {
        m_combatAimEnabled = enabled;
    }

    void AimModeController::SetPaused(bool paused)
    {
        m_paused = paused;

    }

    void AimModeController::SetCursorVisible(bool visible)
    {
        if (m_cursorObject)
            m_cursorObject->SetActive(visible);
    }

    AimModeController::AimCursorState AimModeController::ComputeDesiredCursorState(AimMode mode) const
    {
        const bool leftDown = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT) ||
            engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

        if (mode == AimMode::Pointer)
            return leftDown ? AimCursorState::Clicked : AimCursorState::Default;

        return leftDown ? AimCursorState::AimFiring : AimCursorState::AimIdle; //(여기서 처형 가능이거나, 마우스를 적 위에 올릴 시)
    }
    AimModeController::AimMode AimModeController::ComputeEffectiveMode() const
    {
        if (m_paused)
            return AimMode::Pointer;

        if (m_combatAimEnabled)
            return AimMode::CombatAim;

        return m_baseMode;
    }

    void AimModeController::SetCursorTexture(AimCursorState state)
    {
        EnsureUICursor();
        if (!m_cursorImage || !m_cursorRect) return;

        const std::string& path = m_cursorTexByState[(int)state];
        if (!path.empty() && path != "None")
            m_cursorImage->SetTexture(path);

        m_cursorRect->SetPivot(m_cursorPivotByState[(int)state]);
        m_cursorRect->SetSize(m_cursorSize.x, m_cursorSize.y);

        m_cursor = state;
    }

    void AimModeController::EnsureUICursor()
    {
        // 이미 초기화되어 있으면 스킵
        if (m_canvas && m_cursorObject && m_cursorImage && m_cursorRect) return;
            
        // 1) Canvas는 반드시 존재해야 함
        auto* canvasGO = engine::GameObject::Find(m_canvasObjectName);
        if (!canvasGO) return;

        m_canvas = canvasGO->GetComponent<engine::Canvas>();
        if (!m_canvas) return;

        auto* cursorGO = engine::GameObject::Find("AimCursor");
        if (!cursorGO) return;

        m_cursorObject = cursorGO;

        m_cursorImage = cursorGO->GetComponent<engine::UIImage>();
        if (!m_cursorImage)
            m_cursorImage = cursorGO->AddComponent<engine::UIImage>();

        if (m_cursorImage)
        {
            m_cursorImage->m_raycastTarget = false;
            m_cursorImage->SetAlphaBlend(true);
        }

        m_cursorRect = cursorGO->GetComponent<engine::RectTransform>();
        if (!m_cursorRect) return;

        m_cursorRect->SetAnchorMin({ 0.5f, 0.5f });
        m_cursorRect->SetAnchorMax({ 0.5f, 0.5f });
    }

    void AimModeController::UpdateWorldPositionFromMouse(const engine::Vector2& mousePos)
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

        // 서서 쏠 때는 보정 0, 걸을 때만 m_aimYOffsetWhenMoving 적용
        float effectiveOffset = m_isMoving ? m_aimYOffsetWhenMoving : 0.0f;
        float planeY = m_targetPlaneY + effectiveOffset;
        if (std::abs(rayDir.y) > 0.0001f)
        {
            float t = (planeY - rayOrigin.y) / rayDir.y;

            if (t > 0.0f)
            {
                m_worldPosition = rayOrigin + rayDir * t;
                m_worldPosition.y = planeY;
            }
            else
            {
                m_worldPosition = engine::Vector3::Zero;
            }
        }
        else
        {
            m_worldPosition = engine::Vector3::Zero;
        }
    }

    void AimModeController::TickWorldAim(const engine::Vector2& mousePx, AimMode mode)
    {
        //if (mode != AimMode::CombatAim)
        //    return;

        UpdateWorldPositionFromMouse(mousePx);
    }

    void AimModeController::TickUICursor(const engine::Vector2& mousePx, AimMode mode)
    {
        EnsureUICursor();
        if (!m_canvas || !m_cursorObject) return;

        SetCursorVisible(true);

        const engine::Vector2 s = m_canvas->GetUIScale();
        const engine::Vector2 o = m_canvas->GetUIOffset();
        if (s.x == 0.0f || s.y == 0.0f) return;

        engine::Vector2 mouseRefTL{
            (mousePx.x - o.x) / s.x,
            (mousePx.y - o.y) / s.y
        };

        const engine::Vector2 ref = m_canvas->GetReferenceResolution();
        engine::Vector2 mouseRefCenter{
            mouseRefTL.x - ref.x * 0.5f,
            mouseRefTL.y - ref.y * 0.5f
        };

        m_cursorRect->SetAnchoredPosition(mouseRefCenter);

        // 클릭시 변환
        const AimCursorState desired = ComputeDesiredCursorState(mode);
        if (desired != m_cursor)
            SetCursorTexture(desired);

        // Debug
        //if (engine::Input::IsKeyPressed(engine::Keys::P))
        //{
        //    m_debugIndex++;
        //    m_debugIndex = m_debugIndex % (int)AimCursorState::Count;
        //    SetCursorTexture((AimCursorState)m_debugIndex);
        //}
    }

    engine::Vector3 AimModeController::GetDirectionFrom(const engine::Vector3& fromPosition) const
    {
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.y = 0.0f;  // XZ 평면에서의 방향 (Y축 무시)
        direction.Normalize();
        return direction;
    }

    void AimModeController::OnGui()
    {      
        ImGui::Text("World Position: (%.2f, %.2f, %.2f)",
            m_worldPosition.x, m_worldPosition.y, m_worldPosition.z);

        ImGui::Separator();
        ImGui::Text("Raycast Settings");
        ImGui::DragFloat("Target Plane Y", &m_targetPlaneY, 0.1f, -100.0f, 100.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("The Y height of the plane to raycast against.\nSet this to match your bullet firing height.");
        }
        ImGui::DragFloat("Aim Y Offset (when moving)", &m_aimYOffsetWhenMoving, 0.05f, -5.0f, 5.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("걸을 때만 적용되는 에임 Y 보정. 서서 쏠 때는 0 (Target Plane Y로 조정). 양수=위, 음수=아래");
        ImGui::Text("Moving: %s", m_isMoving ? "Yes" : "No");

        ImGui::Separator();
        ImGui::Text("UI Cursor");
        ImGui::Text("Press P to swap cursor image.");

        std::string selectedTex[5] = {};
        static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga" };

        if (engine::DrawFileSelector("Default", "Resource/Texture/UI/Cursor", texExtensions, selectedTex[0]))
        {
            m_cursorTexByState[0] = selectedTex[0];
        }
        ImGui::SameLine();
        ImGui::Text("Texture: %s", std::filesystem::path(m_cursorTexByState[0]).filename().string().c_str());

        if (engine::DrawFileSelector("Clicked", "Resource/Texture/UI/Cursor", texExtensions, selectedTex[1]))
        {
            m_cursorTexByState[1] = selectedTex[1];
        }
        ImGui::SameLine();
        ImGui::Text("Texture: %s", std::filesystem::path(m_cursorTexByState[1]).filename().string().c_str());

        if (engine::DrawFileSelector("AimIdle", "Resource/Texture/UI/Cursor", texExtensions, selectedTex[2]))
        {
            m_cursorTexByState[2] = selectedTex[2];
        }
        ImGui::SameLine();
        ImGui::Text("Texture: %s", std::filesystem::path(m_cursorTexByState[2]).filename().string().c_str());
        
        if (engine::DrawFileSelector("AimFiring", "Resource/Texture/UI/Cursor", texExtensions, selectedTex[3]))
        {
            m_cursorTexByState[3] = selectedTex[3];
        }
        ImGui::SameLine();
        ImGui::Text("Texture: %s", std::filesystem::path(m_cursorTexByState[3]).filename().string().c_str());

        if (engine::DrawFileSelector("AimExecute", "Resource/Texture/UI/Cursor", texExtensions, selectedTex[4]))
        {
            m_cursorTexByState[4] = selectedTex[4];
        }
        ImGui::SameLine();
        ImGui::Text("Texture: %s", std::filesystem::path(m_cursorTexByState[4]).filename().string().c_str());

        if (ImGui::DragFloat2("Cursor Size", &m_cursorSize.x, 1.0f, 1.0f, 1024.0f))
        {
            if (m_cursorRect)
                m_cursorRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        }
    }

    void AimModeController::Save(engine::json& j) const
    {        
        Object::Save(j);

        j["CanvasObjectName"] = m_canvasObjectName;
        j["CursorSize"] = m_cursorSize;
        //j["CursorPivot"] = m_cursorPivot;

        j["TargetPlaneY"] = m_targetPlaneY;
        j["AimYOffsetWhenMoving"] = m_aimYOffsetWhenMoving;

        // 상태별 텍스처 배열 저장
        engine::json cursorTextures = engine::json::array();
        for (int i = 0; i < (int)AimCursorState::Count; ++i)
        {
            cursorTextures.push_back(m_cursorTexByState[i]);
        }
        j["CursorTexturesByState"] = cursorTextures;

        engine::json cursorPivots = engine::json::array();
        for (int i = 0; i < (int)AimCursorState::Count; ++i)
            cursorPivots.push_back(m_cursorPivotByState[i]);
        j["CursorPivotsByState"] = cursorPivots;

        j["CursorState"] = (int)m_cursor;
    }

    void AimModeController::Load(const engine::json& j)
    {      
        Object::Load(j);

        engine::JsonGet(j, "CanvasObjectName", m_canvasObjectName);

        engine::JsonGet(j, "CursorSize", m_cursorSize);
        //engine::JsonGet(j, "CursorPivot", m_cursorPivot);
        engine::JsonGet(j, "TargetPlaneY", m_targetPlaneY);
        engine::JsonGet(j, "AimYOffsetWhenMoving", m_aimYOffsetWhenMoving);
        if (j.contains("AimYOffset") && !j.contains("AimYOffsetWhenMoving"))
            m_aimYOffsetWhenMoving = j["AimYOffset"].get<float>();

        int state = (int)AimCursorState::Default;
        engine::JsonGet(j, "CursorState", state);
        m_cursor = (AimCursorState)state;

        {
            int idx = 0;
            std::fill(m_cursorTexByState.begin(), m_cursorTexByState.end(), std::string{});
            engine::JsonArrayForEach(j, "CursorTexturesByState",
                [this, &idx](const engine::json& v)
                {
                    if (idx >= (int)AimCursorState::Count)
                        return;

                    m_cursorTexByState[idx] = v.get<std::string>();
                    ++idx;
                }
            );
        }

        {
            int idx = 0;
            for (int i = 0; i < (int)AimCursorState::Count; ++i)
                m_cursorPivotByState[i] = m_cursorPivot; // 기본값으로 초기화

            engine::JsonArrayForEach(j, "CursorPivotsByState",
                [this, &idx](const engine::json& v)
                {
                    if (idx >= (int)AimCursorState::Count) return;
                    m_cursorPivotByState[idx] = v.get<engine::Vector2>();
                    ++idx;
                });
        }

        EnsureUICursor();
        SetCursorTexture(m_cursor);
    }
}