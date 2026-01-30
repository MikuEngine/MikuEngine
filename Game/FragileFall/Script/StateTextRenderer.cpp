#include "GamePCH.h"
#include "StateTextRenderer.h"

#include "Script/CharacterScript/Common/BaseControllerScript.h"
#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Core/Graphics/Device/GraphicsDevice.h>

#include <Framework/Asset/Prefab.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Camera.h>
#include <Framework/Object/Component/Canvas.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/LogicFSM.h>

namespace game
{
    void StateTextRenderer::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        // Canvas 컴포넌트 캐싱
        m_canvas = go->GetComponent<engine::Canvas>();
        if (!m_canvas)
        {
            LOG_PRINT("[StateTextRenderer] WARNING: Canvas component not found!");
        }

        // 부모 RectTransform 찾기
        if (auto* rt = go->GetComponent<engine::RectTransform>())
        {
            engine::RectTransform* parentRT = rt->FindPrentRectTransform();
            m_parentRT = parentRT ? parentRT : rt;  // 자신이 루트면 자신을 사용
        }
    }

    void StateTextRenderer::Start()
    {
        // 메인 카메라 찾기
        if (auto* camGO = engine::GameObject::Find("MainCamera"))
        {
            m_mainCamera = camGO->GetComponent<engine::Camera>();
        }

        if (!m_mainCamera)
        {
            LOG_PRINT("[StateTextRenderer] WARNING: MainCamera not found!");
        }

        // 씬의 모든 BaseControllerScript 찾기
        FindAllControllers();
    }

    void StateTextRenderer::Update()
    {
#ifdef _DEBUG
        if (!m_mainCamera) return;

        // 파괴된 오브젝트 정리
        CleanupDestroyedObjects();

        // 각 추적 대상 업데이트
        for (auto& tracked : m_trackedObjects)
        {
            UpdateTrackedObject(tracked);
        }
#else
        // 릴리즈 빌드에서는 모든 텍스트 숨김
        for (auto& tracked : m_trackedObjects)
        {
            SetTextVisible(tracked, false);
        }
#endif
    }

    void StateTextRenderer::FindAllControllers()
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;

        // 씬의 모든 게임오브젝트 순회
        for (const auto& go : scene->GetGameObjects())
        {
            // BaseControllerScript를 상속한 모든 컴포넌트 찾기
            if (auto* controller = go->GetComponent<BaseControllerScript>())
            {
                // 이미 추적 중인지 확인
                bool alreadyTracked = false;
                for (const auto& tracked : m_trackedObjects)
                {
                    // Ptr 비교: Handle 기반으로 비교됨
                    if (tracked.controller && tracked.controller.Get() == controller)
                    {
                        alreadyTracked = true;
                        break;
                    }
                }

                if (!alreadyTracked)
                {
                    CreateTextForController(controller);
                }
            }
        }
    }

    void StateTextRenderer::CreateTextForController(BaseControllerScript* controller)
    {
        if (!controller) return;

        // StateText 프리팹 인스턴스화
        engine::GameObject* textGO = engine::Prefab::Instantiate(m_prefabName);
        if (!textGO)
        {
            LOG_PRINT("[StateTextRenderer] ERROR: Failed to instantiate prefab '%s'", m_prefabName.c_str());
            return;
        }

        // Canvas가 있는 오브젝트 (this)의 자식으로 설정 (UI 렌더링을 위해 필수)
        if (GetGameObject() && GetGameObject()->GetTransform())
        {
            textGO->GetTransform()->SetParent(GetGameObject()->GetTransform(), false);
        }

        // UIText 컴포넌트 가져오기
        engine::UIText* uiText = textGO->GetComponent<engine::UIText>();
        engine::RectTransform* rectTransform = textGO->GetComponent<engine::RectTransform>();

        if (!uiText || !rectTransform)
        {
            LOG_PRINT("[StateTextRenderer] ERROR: Prefab '%s' missing UIText or RectTransform!", m_prefabName.c_str());
            textGO->Destroy();
            return;
        }

        // 앵커 설정 (좌상단 기준)
        rectTransform->SetAnchorMin({ 0.0f, 0.0f });
        rectTransform->SetAnchorMax({ 0.0f, 0.0f });
        rectTransform->SetPivot({ 0.5f, 0.5f });
        
        // 상태 텍스트 폰트 크기 설정
        uiText->SetFontPixelSize(m_stateFontSize);

        // TrackedObject 추가 (Ptr로 안전하게 저장)
        TrackedObject tracked;
        tracked.controller = controller;
        tracked.textObject = textGO;
        tracked.uiText = uiText;
        tracked.rectTransform = rectTransform;
        
        // MonsterScript 또는 PlayerControllerScript인 경우 체력 텍스트도 생성
        MonsterScript* monster = dynamic_cast<MonsterScript*>(controller);
        PlayerControllerScript* player = dynamic_cast<PlayerControllerScript*>(controller);
        
        if (m_showHpText && (monster || player))
        {
            engine::GameObject* hpTextGO = engine::Prefab::Instantiate(m_prefabName);
            if (hpTextGO)
            {
                if (GetGameObject() && GetGameObject()->GetTransform())
                {
                    hpTextGO->GetTransform()->SetParent(GetGameObject()->GetTransform(), false);
                }
                
                engine::UIText* hpUiText = hpTextGO->GetComponent<engine::UIText>();
                engine::RectTransform* hpRectTransform = hpTextGO->GetComponent<engine::RectTransform>();
                
                if (hpUiText && hpRectTransform)
                {
                    hpRectTransform->SetAnchorMin({ 0.0f, 0.0f });
                    hpRectTransform->SetAnchorMax({ 0.0f, 0.0f });
                    hpRectTransform->SetPivot({ 0.5f, 0.5f });
                    
                    // 체력 텍스트는 더 큰 폰트
                    hpUiText->SetFontPixelSize(m_hpFontSize);
                    hpUiText->SetAlignH(engine::UITextAlignH::Center);
                    
                    tracked.hpTextObject = hpTextGO;
                    tracked.hpUiText = hpUiText;
                    tracked.hpRectTransform = hpRectTransform;
                }
                else
                {
                    hpTextGO->Destroy();
                }
            }
        }
        
        // PlayerControllerScript인 경우 대쉬 텍스트도 생성
        if (m_showDashText && player)
        {
            engine::GameObject* dashTextGO = engine::Prefab::Instantiate(m_prefabName);
            if (dashTextGO)
            {
                if (GetGameObject() && GetGameObject()->GetTransform())
                {
                    dashTextGO->GetTransform()->SetParent(GetGameObject()->GetTransform(), false);
                }
                
                engine::UIText* dashUiText = dashTextGO->GetComponent<engine::UIText>();
                engine::RectTransform* dashRectTransform = dashTextGO->GetComponent<engine::RectTransform>();
                
                if (dashUiText && dashRectTransform)
                {
                    dashRectTransform->SetAnchorMin({ 0.0f, 0.0f });
                    dashRectTransform->SetAnchorMax({ 0.0f, 0.0f });
                    dashRectTransform->SetPivot({ 0.5f, 0.5f });
                    
                    dashUiText->SetFontPixelSize(m_dashFontSize);
                    dashUiText->SetAlignH(engine::UITextAlignH::Center);
                    
                    tracked.dashTextObject = dashTextGO;
                    tracked.dashUiText = dashUiText;
                    tracked.dashRectTransform = dashRectTransform;
                }
                else
                {
                    dashTextGO->Destroy();
                }
            }
        }

        m_trackedObjects.push_back(tracked);
    }

    void StateTextRenderer::UpdateTrackedObject(TrackedObject& tracked)
    {
        // Ptr 유효성 검사 (댕글링 자동 처리)
        if (!tracked.controller || !tracked.textObject || !tracked.uiText || !tracked.rectTransform)
        {
            SetTextVisible(tracked, false);
            return;
        }

        // 컨트롤러의 게임오브젝트가 유효한지 확인
        engine::GameObject* targetGO = tracked.controller->GetGameObject();
        if (!targetGO)
        {
            SetTextVisible(tracked, false);
            return;
        }

        // LogicFSM에서 현재 상태 가져오기
        engine::LogicFSM* logicFSM = targetGO->GetComponent<engine::LogicFSM>();
        if (!logicFSM)
        {
            SetTextVisible(tracked, false);
            return;
        }

        std::string currentState = logicFSM->GetCurrentState();
        tracked.uiText->SetText(currentState);

        // 월드 좌표 계산 (오브젝트 위치 + 오프셋)
        engine::Vector3 worldPos = targetGO->GetTransform()->GetWorldPosition() + m_worldOffset;

        // 스크린 좌표로 변환
        engine::Vector2 screenPos;
        if (!WorldToScreen(worldPos, screenPos))
        {
            // 카메라 뒤에 있거나 절두체 밖
            SetTextVisible(tracked, false);
            return;
        }

        // UI 위치 설정 (상태 텍스트)
        const engine::Vector2 finalPos(
            screenPos.x - m_cachedParentRectX,
            screenPos.y - m_cachedParentRectY
        );

        tracked.rectTransform->SetAnchoredPosition(finalPos);
        
        // 체력 텍스트 업데이트 (MonsterScript 또는 PlayerControllerScript인 경우)
        if (tracked.hpUiText && tracked.hpRectTransform)
        {
            std::string hpText;
            
            // 플레이어인 경우
            PlayerControllerScript* player = dynamic_cast<PlayerControllerScript*>(tracked.controller.Get());
            if (player)
            {
                // "현재HP / 맥스HP" 형식
                hpText = std::to_string(player->GetCurrentHp()) + " / " + std::to_string(player->GetMaxHp());
            }
            else
            {
                // 몬스터인 경우
                MonsterScript* monster = dynamic_cast<MonsterScript*>(tracked.controller.Get());
                if (monster)
                {
                    hpText = "HP: " + std::to_string(monster->GetHp());
                }
            }
            
            if (!hpText.empty())
            {
                tracked.hpUiText->SetText(hpText);
                
                // 체력 텍스트 위치 (상태 텍스트 위쪽)
                const engine::Vector2 hpPos(
                    finalPos.x,
                    finalPos.y - m_hpTextYOffset  // 위로 올림 (스크린 좌표계에서 Y가 아래로 증가)
                );
                tracked.hpRectTransform->SetAnchoredPosition(hpPos);
            }
        }
        
        // 대쉬 텍스트 업데이트 (PlayerControllerScript인 경우)
        if (tracked.dashUiText && tracked.dashRectTransform)
        {
            PlayerControllerScript* player = dynamic_cast<PlayerControllerScript*>(tracked.controller.Get());
            if (player)
            {
                // 대쉬 카운트와 리차지 타이머 텍스트 생성 (소수점 3자리)
                int currentDash = player->GetCurrentDashCount();
                int maxDash = player->GetMaxDashCount();
                float rechargeTimer = player->GetDashRechargeTimer();
                
                char buffer[64];
                if (currentDash < maxDash)
                {
                    // 충전 중일 때 타이머 표시
                    snprintf(buffer, sizeof(buffer), "Dash: %d/%d (%.3f)", currentDash, maxDash, rechargeTimer);
                }
                else
                {
                    // 최대치일 때 타이머 미표시
                    snprintf(buffer, sizeof(buffer), "Dash: %d/%d", currentDash, maxDash);
                }
                tracked.dashUiText->SetText(buffer);
                
                // 대쉬 텍스트 위치 (상태 텍스트 위쪽, HP 텍스트보다 더 위)
                const engine::Vector2 dashPos(
                    finalPos.x,
                    finalPos.y - m_dashTextYOffset  // 위로 올림
                );
                tracked.dashRectTransform->SetAnchoredPosition(dashPos);
            }
        }
        
        SetTextVisible(tracked, true);
    }

    void StateTextRenderer::SetTextVisible(TrackedObject& tracked, bool visible)
    {
        // 상태 텍스트 표시/숨김
        if (tracked.uiText)
        {
            engine::Vector4 color = tracked.uiText->GetColor();
            color.w = visible ? 1.0f : 0.0f;
            tracked.uiText->SetColor(color);
        }
        
        // 체력 텍스트 표시/숨김
        if (tracked.hpUiText)
        {
            engine::Vector4 hpColor = tracked.hpUiText->GetColor();
            hpColor.w = visible ? 1.0f : 0.0f;
            tracked.hpUiText->SetColor(hpColor);
        }
        
        // 대쉬 텍스트 표시/숨김
        if (tracked.dashUiText)
        {
            engine::Vector4 dashColor = tracked.dashUiText->GetColor();
            dashColor.w = visible ? 1.0f : 0.0f;
            tracked.dashUiText->SetColor(dashColor);
        }
    }

    void StateTextRenderer::CleanupDestroyedObjects()
    {
        // Ptr의 유효성 검사로 파괴된 오브젝트 자동 제거
        m_trackedObjects.erase(
            std::remove_if(m_trackedObjects.begin(), m_trackedObjects.end(),
                [](const TrackedObject& tracked) {
                    // Ptr가 무효화되면 자동으로 false 반환
                    return !tracked.controller || !tracked.textObject;
                }),
            m_trackedObjects.end()
        );
    }

    bool StateTextRenderer::WorldToScreen(const engine::Vector3& worldPos, engine::Vector2& screenPos)
    {
        if (!m_mainCamera) return false;

        engine::Matrix view = m_mainCamera->GetView();
        engine::Matrix proj = m_mainCamera->GetProjection();

        // 월드 → 클립 좌표
        engine::Vector4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        engine::Vector4 viewPos = engine::Vector4::Transform(pos, view);
        engine::Vector4 clipPos = engine::Vector4::Transform(viewPos, proj);

        // 카메라 뒤에 있으면 실패
        if (clipPos.w <= 0.0001f)
        {
            return false;
        }

        // NDC (Normalized Device Coordinates)
        const float invW = 1.0f / clipPos.w;
        const float ndcX = clipPos.x * invW;
        const float ndcY = clipPos.y * invW;

        // 절두체 밖 체크
        if (m_hideWhenOffscreen &&
            (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f))
        {
            return false;
        }

        // 뷰포트 정보 가져오기
        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        const float vpW = vp.Width;
        const float vpH = vp.Height;

        // 뷰포트 변경 시 부모 Rect 재계산
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

        // NDC → 스크린 좌표
        screenPos.x = (ndcX * 0.5f + 0.5f) * vpW;
        screenPos.y = (-ndcY * 0.5f + 0.5f) * vpH;

        return true;
    }

    void StateTextRenderer::OnGui()
    {
        ImGui::InputText("Prefab Name", &m_prefabName);
        ImGui::DragFloat3("World Offset", &m_worldOffset.x, 0.1f);
        ImGui::Checkbox("Hide When Offscreen", &m_hideWhenOffscreen);
        
        ImGui::Separator();
        ImGui::Text("HP Text Settings:");
        ImGui::Checkbox("Show HP Text", &m_showHpText);
        ImGui::DragInt("HP Font Size", &m_hpFontSize, 1, 12, 128);
        ImGui::DragInt("State Font Size", &m_stateFontSize, 1, 12, 128);
        ImGui::DragFloat("HP Text Y Offset", &m_hpTextYOffset, 1.0f, 0.0f, 100.0f);
        
        ImGui::Separator();
        ImGui::Text("Dash Text Settings (Player Only):");
        ImGui::Checkbox("Show Dash Text", &m_showDashText);
        ImGui::DragInt("Dash Font Size", &m_dashFontSize, 1, 12, 128);
        ImGui::DragFloat("Dash Text Y Offset", &m_dashTextYOffset, 1.0f, 0.0f, 200.0f);

        ImGui::Separator();
        ImGui::Text("Tracked Objects: %d", static_cast<int>(m_trackedObjects.size()));

        if (ImGui::Button("Refresh Controllers"))
        {
            FindAllControllers();
        }
    }

    void StateTextRenderer::Save(engine::json& j) const
    {
        Object::Save(j);
        j["PrefabName"] = m_prefabName;
        j["WorldOffset"] = m_worldOffset;
        j["HideWhenOffscreen"] = m_hideWhenOffscreen;
        j["ShowHpText"] = m_showHpText;
        j["HpFontSize"] = m_hpFontSize;
        j["StateFontSize"] = m_stateFontSize;
        j["HpTextYOffset"] = m_hpTextYOffset;
        j["ShowDashText"] = m_showDashText;
        j["DashFontSize"] = m_dashFontSize;
        j["DashTextYOffset"] = m_dashTextYOffset;
    }

    void StateTextRenderer::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "PrefabName", m_prefabName);
        engine::JsonGet(j, "WorldOffset", m_worldOffset);
        engine::JsonGet(j, "HideWhenOffscreen", m_hideWhenOffscreen);
        engine::JsonGet(j, "ShowHpText", m_showHpText);
        engine::JsonGet(j, "HpFontSize", m_hpFontSize);
        engine::JsonGet(j, "StateFontSize", m_stateFontSize);
        engine::JsonGet(j, "HpTextYOffset", m_hpTextYOffset);
        engine::JsonGet(j, "ShowDashText", m_showDashText);
        engine::JsonGet(j, "DashFontSize", m_dashFontSize);
        engine::JsonGet(j, "DashTextYOffset", m_dashTextYOffset);
    }
}
