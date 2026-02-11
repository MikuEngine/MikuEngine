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
#include <Framework/System/SystemManager.h>
#include <Framework/Physics/PhysicsSystem.h>

#include <Core/Graphics/Resource/ResourceManager.h>

#include <Scene/GameScene.h>
#include "Script/Interface/IDamageable.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossBigProjectile.h"
#include <algorithm>
#include <cctype>

namespace game
{
    void AimModeController::Awake()
    {
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
        EnsurePulseBindings();
        ResetCursorPulseSprites();
        EnsureExecutionIntroBindings();
        ResetExecutionIntroPose();
        m_prevOnExecutionTarget = m_isOnExecutionTarget;

        const engine::Vector2 mousePx = engine::Input::GetMousePosition();
        const AimMode mode = ComputeEffectiveMode();

        m_cursor = AimCursorState::Count; // "무효 값"으로 만들어서 아래에서 반드시 ApplyCursorState 호출되게
        const AimCursorState desired = ComputeDesiredCursorState(mode);
        ApplyCursorState(desired);

        TickWorldAim(mousePx, mode);
        TickUICursor(mousePx, mode);
    }

    void AimModeController::Update()
    {
        // 클라이언트 상의 마우스 위치
        engine::Vector2 mousePx = engine::Input::GetMousePosition();
        const AimMode mode = ComputeEffectiveMode();

        TickWorldAim(mousePx, mode);
        UpdateOnEnemyByRaycast(mousePx, mode);
        EnsurePulseBindings();
        UpdateCursorPulse(engine::Time::DeltaTime());
        EnsureExecutionIntroBindings();

        const bool hoverEnteredExecution = (m_isOnExecutionTarget && !m_prevOnExecutionTarget);
        const bool hoverExitedExecution = (!m_isOnExecutionTarget && m_prevOnExecutionTarget);

        if (hoverEnteredExecution)
        {
            StartExecutionIntro();
        }
        else if (hoverExitedExecution && !m_isExecutionInProgress)
        {
            // 처형 대상에서 벗어났고 실제 처형 중이 아니라면 즉시 원상 복귀
            m_executionIntroPlaying = false;
            ResetExecutionIntroPose();
        }

        UpdateExecutionIntro(engine::Time::DeltaTime());
        m_prevOnExecutionTarget = m_isOnExecutionTarget;
        TickUICursor(mousePx, mode);
        
        // 처형 완료 후 타이머 감소
        if (m_postExecutionTimer > 0.0f)
        {
            m_postExecutionTimer -= engine::Time::DeltaTime();
            if (m_postExecutionTimer < 0.0f)
            {
                m_postExecutionTimer = 0.0f;
            }
        }
    }

    void AimModeController::SetCombatAimEnabled(bool enabled)
    {
        m_combatAimEnabled = enabled;
    }

    void AimModeController::SetPaused(bool paused)
    {
        m_paused = paused;

    }

    AimModeController::AimCursorState AimModeController::ComputeDesiredCursorState(AimMode mode) const
    {
        const bool leftDown = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT) ||
            engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

        if (mode == AimMode::Pointer)
            return leftDown ? AimCursorState::Clicked : AimCursorState::Default;

        // ═══════════════════════════════════════════════════════════════
        // AimExecute 표시 조건:
        // 1. 처형 대상 몬스터 위에 마우스
        // 2. 처형 진행 중 (시작~Execution 상태 종료)
        // 3. 처형 완료 후 유지 타이머 동안
        // ═══════════════════════════════════════════════════════════════
        if (m_isExecutionInProgress)
            return AimCursorState::AimExecute;

        if (m_isOnExecutionTarget)
            return AimCursorState::AimExecute;
        
        if (m_postExecutionTimer > 0.0f)
            return AimCursorState::AimExecute;

        if (m_isOnEnemyTarget)
            return AimCursorState::AimOnEnemy;

        return AimCursorState::AimIdle;
    }
    AimModeController::AimMode AimModeController::ComputeEffectiveMode() const
    {
        if (m_paused)
            return AimMode::Pointer;

        if (m_combatAimEnabled)
            return AimMode::CombatAim;       

        return m_baseMode;
    }

    void AimModeController::ApplyCursorState(AimCursorState state)
    {
        EnsureUICursor();
        if (!m_canvas) return;

        // 모든 커서를 우선 비활성화
        if (m_cursorAimObject) m_cursorAimObject->SetActive(false);
        if (m_cursorExecutionObject) m_cursorExecutionObject->SetActive(false);
        if (m_cursorDefaultObject) m_cursorDefaultObject->SetActive(false);
        if (m_cursorClickObject) m_cursorClickObject->SetActive(false);
        if (m_cursorOnEnemyObject) m_cursorOnEnemyObject->SetActive(false);

        // 상태별 활성화 대상 선택
        engine::GameObject* target = nullptr;
        switch (state)
        {
        case AimCursorState::Default:
            target = m_cursorDefaultObject;
            break;
        case AimCursorState::Clicked:
            target = m_cursorClickObject;
            break;
        case AimCursorState::AimIdle:
            target = m_cursorAimObject;
            break;
        case AimCursorState::AimOnEnemy:
            target = m_cursorOnEnemyObject ? m_cursorOnEnemyObject : m_cursorAimObject;
            break;
        case AimCursorState::AimExecute:
            target = m_cursorExecutionObject;
            break;
        default:
            break;
        }

        if (target)
        {
            target->SetActive(true);
        }

        m_cursor = state;
    }

    void AimModeController::EnsureUICursor()
    {
        // 이미 초기화되어 있으면 스킵
        if (m_canvas && m_cursorAimObject && m_cursorExecutionObject && m_cursorDefaultObject && m_cursorClickObject
            && m_cursorAimRect && m_cursorExecutionRect && m_cursorDefaultRect && m_cursorClickRect)
        {
            return;
        }
            
        // 1) Canvas는 반드시 존재해야 함
        auto* canvasGO = engine::GameObject::Find(kCanvasObjectName);
        if (!canvasGO) return;

        m_canvas = canvasGO->GetComponent<engine::Canvas>();
        if (!m_canvas) return;

        m_cursorAimObject = engine::GameObject::Find(kCursorAimName);
        m_cursorExecutionObject = engine::GameObject::Find(kCursorExecutionName);
        m_cursorDefaultObject = engine::GameObject::Find(kCursorDefaultName);
        m_cursorClickObject = engine::GameObject::Find(kCursorClickName);
        m_cursorOnEnemyObject = engine::GameObject::Find(kCursorOnEnemyName);

        m_cursorAimRect = m_cursorAimObject ? m_cursorAimObject->GetComponent<engine::RectTransform>() : nullptr;
        m_cursorExecutionRect = m_cursorExecutionObject ? m_cursorExecutionObject->GetComponent<engine::RectTransform>() : nullptr;
        m_cursorDefaultRect = m_cursorDefaultObject ? m_cursorDefaultObject->GetComponent<engine::RectTransform>() : nullptr;
        m_cursorClickRect = m_cursorClickObject ? m_cursorClickObject->GetComponent<engine::RectTransform>() : nullptr;
        m_cursorOnEnemyRect = m_cursorOnEnemyObject ? m_cursorOnEnemyObject->GetComponent<engine::RectTransform>() : nullptr;

        // 직렬화된 픽셀 크기를 즉시 적용한다.
        if (m_cursorAimRect) m_cursorAimRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        if (m_cursorExecutionRect) m_cursorExecutionRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        if (m_cursorDefaultRect) m_cursorDefaultRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        if (m_cursorClickRect) m_cursorClickRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        if (m_cursorOnEnemyRect) m_cursorOnEnemyRect->SetSize(m_cursorSize.x, m_cursorSize.y);
    }

    void AimModeController::EnsurePulseBindings()
    {
        if (!m_playerController)
        {
            m_playerController = GetGameObject() ? GetGameObject()->GetComponent<PlayerControllerScript>() : nullptr;
        }

        if (m_playerController && !m_fireCallbackRegistered)
        {
            m_playerController->RegisterFireCallback(this, [this]() { OnPlayerFired(); });
            m_fireCallbackRegistered = true;
        }

        if (!m_pulseSpritesInitialized)
        {
            EnsureUICursor();
            CollectPulseSprites(m_cursorAimObject, m_pulseAimSprites, m_pulseAimBasePositions, m_pulseAimDirections);
            CollectPulseSprites(m_cursorOnEnemyObject, m_pulseOnEnemySprites, m_pulseOnEnemyBasePositions, m_pulseOnEnemyDirections);
            const bool hasAnyRoot = (m_cursorAimObject != nullptr) || (m_cursorOnEnemyObject != nullptr);
            if (hasAnyRoot)
            {
                m_pulseSpritesInitialized = true;
                ApplyCursorPulseToSprites();
            }
        }
    }

    void AimModeController::CollectPulseSprites(
        engine::GameObject* root,
        std::vector<engine::RectTransform*>& outSprites,
        std::vector<engine::Vector2>& outBasePositions,
        std::vector<engine::Vector2>& outDirections)
    {
        outSprites.clear();
        outBasePositions.clear();
        outDirections.clear();

        if (root == nullptr || root->GetTransform() == nullptr)
            return;

        const std::vector<engine::Transform*>& children = root->GetTransform()->GetChildren();
        outSprites.reserve(children.size());
        outBasePositions.reserve(children.size());
        outDirections.reserve(children.size());

        auto suffixDir = [](const std::string& name, engine::Vector2& outDir) -> bool
            {
                if (name.size() < 2 || name[name.size() - 2] != '_')
                    return false;

                const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(name.back())));
                switch (c)
                {
                case 'N': outDir = engine::Vector2(0.0f, -1.0f); return true;
                case 'S': outDir = engine::Vector2(0.0f, 1.0f); return true;
                case 'W': outDir = engine::Vector2(-1.0f, 0.0f); return true;
                case 'E': outDir = engine::Vector2(1.0f, 0.0f); return true;
                default: return false;
                }
            };

        for (engine::Transform* child : children)
        {
            if (!child || !child->GetGameObject())
                continue;

            auto* rt = child->GetGameObject()->GetComponent<engine::RectTransform>();
            if (!rt)
                continue;

            engine::Vector2 dir;
            if (!suffixDir(child->GetGameObject()->GetName(), dir))
            {
                continue;
            }

            outSprites.push_back(rt);
            outBasePositions.push_back(rt->GetAnchoredPosition());
            outDirections.push_back(dir);
        }
    }

    void AimModeController::OnPlayerFired()
    {
        if (!m_enableCursorPulse)
            return;

        EnsurePulseBindings();

        float effectiveFireRate = 0.1f;
        if (m_playerController)
            effectiveFireRate = m_playerController->GetEffectiveFireRate();

        const float totalDuration = std::clamp(effectiveFireRate * m_pulseDurationPerFR, 0.02f, 5.0f);
        const float peakRatio = std::clamp(m_pulsePeakTimeScale, 0.01f, 0.99f);

        m_pulseOutDuration = std::max(0.005f, totalDuration * peakRatio);
        m_pulseBackDuration = std::max(0.005f, totalDuration - m_pulseOutDuration);

        m_isPulsePlaying = true;
        m_isPulseMovingOutward = true;
        m_pulseCurrent = std::clamp(m_pulseCurrent, 0.0f, 1.0f);
    }

    void AimModeController::UpdateCursorPulse(float deltaTime)
    {
        if (!m_enableCursorPulse)
        {
            if (m_isPulsePlaying || m_pulseCurrent > 0.0f)
            {
                m_isPulsePlaying = false;
                m_isPulseMovingOutward = false;
                m_pulseCurrent = 0.0f;
                ResetCursorPulseSprites();
            }
            return;
        }

        if (!m_isPulsePlaying)
            return;

        if (deltaTime <= 0.0f)
        {
            ApplyCursorPulseToSprites();
            return;
        }

        if (m_isPulseMovingOutward)
        {
            m_pulseCurrent += (deltaTime / m_pulseOutDuration);
            if (m_pulseCurrent >= 1.0f)
            {
                m_pulseCurrent = 1.0f;
                m_isPulseMovingOutward = false;
            }
        }
        else
        {
            m_pulseCurrent -= (deltaTime / m_pulseBackDuration);
            if (m_pulseCurrent <= 0.0f)
            {
                m_pulseCurrent = 0.0f;
                m_isPulsePlaying = false;
            }
        }

        m_pulseCurrent = std::clamp(m_pulseCurrent, 0.0f, 1.0f);
        ApplyCursorPulseToSprites();
    }

    void AimModeController::ApplyCursorPulseToSprites()
    {
        const float distance = std::max(0.0f, m_pulseMaxDistance) * m_pulseCurrent;

        auto apply = [distance](const std::vector<engine::RectTransform*>& sprites,
            const std::vector<engine::Vector2>& bases,
            const std::vector<engine::Vector2>& dirs)
            {
                const size_t count = std::min(sprites.size(), std::min(bases.size(), dirs.size()));
                for (size_t i = 0; i < count; ++i)
                {
                    engine::RectTransform* rt = sprites[i];
                    if (!rt || !rt->GetGameObject())
                        continue;

                    rt->SetAnchoredPosition(bases[i] + dirs[i] * distance);
                }
            };

        apply(m_pulseAimSprites, m_pulseAimBasePositions, m_pulseAimDirections);
        apply(m_pulseOnEnemySprites, m_pulseOnEnemyBasePositions, m_pulseOnEnemyDirections);
    }

    void AimModeController::ResetCursorPulseSprites()
    {
        m_pulseCurrent = 0.0f;
        ApplyCursorPulseToSprites();
    }

    void AimModeController::EnsureExecutionIntroBindings()
    {
        if (m_executionIntroInitialized)
            return;

        EnsureUICursor();
        if (!m_cursorExecutionObject || !m_cursorExecutionObject->GetTransform())
            return;

        const std::vector<engine::Transform*>& children = m_cursorExecutionObject->GetTransform()->GetChildren();
        m_executionIntroSprites.clear();
        m_executionIntroBasePositions.clear();
        m_executionIntroBaseScales.clear();
        m_executionIntroDirections.clear();
        m_executionIntroSprites.reserve(children.size());
        m_executionIntroBasePositions.reserve(children.size());
        m_executionIntroBaseScales.reserve(children.size());
        m_executionIntroDirections.reserve(children.size());

        auto suffixDir = [](const std::string& name, engine::Vector2& outDir) -> bool
            {
                const size_t pos = name.rfind('_');
                if (pos == std::string::npos || pos + 1 >= name.size())
                    return false;

                std::string suffix = name.substr(pos + 1);
                for (char& c : suffix)
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

                if (suffix == "NW")
                {
                    outDir = engine::Vector2(-1.0f, -1.0f);
                    return true;
                }
                if (suffix == "NE")
                {
                    outDir = engine::Vector2(1.0f, -1.0f);
                    return true;
                }
                if (suffix == "SE")
                {
                    outDir = engine::Vector2(1.0f, 1.0f);
                    return true;
                }
                if (suffix == "SW")
                {
                    outDir = engine::Vector2(-1.0f, 1.0f);
                    return true;
                }
                return false;
            };

        for (engine::Transform* child : children)
        {
            if (!child || !child->GetGameObject())
                continue;

            auto* rt = child->GetGameObject()->GetComponent<engine::RectTransform>();
            if (!rt)
                continue;

            engine::Vector2 dir;
            if (!suffixDir(child->GetGameObject()->GetName(), dir))
                continue;

            if (dir.LengthSquared() > 0.0001f)
                dir.Normalize();

            m_executionIntroSprites.push_back(rt);
            m_executionIntroBasePositions.push_back(rt->GetAnchoredPosition());
            m_executionIntroBaseScales.push_back(rt->GetLocalScale());
            m_executionIntroDirections.push_back(dir);
        }

        m_executionIntroInitialized = true;
    }

    void AimModeController::StartExecutionIntro()
    {
        EnsureExecutionIntroBindings();
        if (m_executionIntroSprites.empty())
            return;

        m_executionIntroTimer = 0.0f;
        m_executionIntroPlaying = true;
        ApplyExecutionIntroPose(1.0f);
    }

    void AimModeController::UpdateExecutionIntro(float deltaTime)
    {
        if (!m_executionIntroPlaying)
            return;

        // 처형이 이미 끝났고 처형 대상도 아니라면 즉시 종료/복귀
        if (!m_isExecutionInProgress && !m_isOnExecutionTarget)
        {
            m_executionIntroPlaying = false;
            ResetExecutionIntroPose();
            return;
        }

        if (m_executionIntroDuration <= 0.0001f)
        {
            m_executionIntroPlaying = false;
            ResetExecutionIntroPose();
            return;
        }

        m_executionIntroTimer += std::max(0.0f, deltaTime);
        const float t = std::clamp(m_executionIntroTimer / m_executionIntroDuration, 0.0f, 1.0f);
        const float alpha = 1.0f - t; // 시작(1) -> 종료(0)

        ApplyExecutionIntroPose(alpha);
        if (t >= 1.0f)
        {
            m_executionIntroPlaying = false;
            ResetExecutionIntroPose();
        }
    }

    void AimModeController::ApplyExecutionIntroPose(float alpha)
    {
        const float clamped = std::clamp(alpha, 0.0f, 1.0f);
        const float scaleMul = 1.0f + (std::max(1.0f, m_executionIntroScale) - 1.0f) * clamped;
        const float distance = std::max(0.0f, m_executionIntroDistancePx) * clamped;

        const size_t count = std::min(
            m_executionIntroSprites.size(),
            std::min(m_executionIntroBasePositions.size(),
                std::min(m_executionIntroBaseScales.size(), m_executionIntroDirections.size())));

        for (size_t i = 0; i < count; ++i)
        {
            auto* rt = m_executionIntroSprites[i];
            if (!rt || !rt->GetGameObject())
                continue;

            rt->SetAnchoredPosition(m_executionIntroBasePositions[i] + m_executionIntroDirections[i] * distance);
            rt->SetLocalScale(m_executionIntroBaseScales[i] * scaleMul);
        }
    }

    void AimModeController::ResetExecutionIntroPose()
    {
        const size_t count = std::min(
            m_executionIntroSprites.size(),
            std::min(m_executionIntroBasePositions.size(), m_executionIntroBaseScales.size()));

        for (size_t i = 0; i < count; ++i)
        {
            auto* rt = m_executionIntroSprites[i];
            if (!rt || !rt->GetGameObject())
                continue;

            rt->SetAnchoredPosition(m_executionIntroBasePositions[i]);
            rt->SetLocalScale(m_executionIntroBaseScales[i]);
        }
    }

    void AimModeController::UpdateWorldPositionFromMouse(const engine::Vector2& mousePos)
    {
        engine::Vector3 rayOrigin;
        engine::Vector3 rayDir;
        if (!TryBuildMouseRayFromScreen(mousePos, rayOrigin, rayDir))
        {
            m_worldPosition = engine::Vector3::Zero;
            return;
        }

        // 이동 중일 때는 추가 오프셋을 적용한다. (예: 1.7 -> 1.3)
        float planeY = m_targetPlaneY + (m_isMoving ? m_aimYOffsetWhenMoving : 0.0f);
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

    bool AimModeController::TryBuildMouseRayFromScreen(const engine::Vector2& mousePos, engine::Vector3& outRayOrigin, engine::Vector3& outRayDir) const
    {
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
            return false;

        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        const float screenWidth = (vp.Width > 0.0f) ? vp.Width : 1920.0f;
        const float screenHeight = (vp.Height > 0.0f) ? vp.Height : 1080.0f;

        const float ndcX = (2.0f * mousePos.x / screenWidth) - 1.0f;
        const float ndcY = 1.0f - (2.0f * mousePos.y / screenHeight);

        engine::Matrix invProj = camera->GetProjection().Invert();
        engine::Matrix invView = camera->GetView().Invert();

        engine::Vector4 nearPointNDC(ndcX, ndcY, 0.0f, 1.0f);
        engine::Vector4 farPointNDC(ndcX, ndcY, 1.0f, 1.0f);

        engine::Vector4 nearPointView = engine::Vector4::Transform(nearPointNDC, invProj);
        engine::Vector4 farPointView = engine::Vector4::Transform(farPointNDC, invProj);

        if (std::abs(nearPointView.w) > 0.0001f)
            nearPointView /= nearPointView.w;
        if (std::abs(farPointView.w) > 0.0001f)
            farPointView /= farPointView.w;

        engine::Vector4 nearPointWorld = engine::Vector4::Transform(nearPointView, invView);
        engine::Vector4 farPointWorld = engine::Vector4::Transform(farPointView, invView);

        outRayOrigin = engine::Vector3(nearPointWorld.x, nearPointWorld.y, nearPointWorld.z);
        engine::Vector3 rayEnd(farPointWorld.x, farPointWorld.y, farPointWorld.z);
        outRayDir = rayEnd - outRayOrigin;

        if (outRayDir.LengthSquared() < 0.0001f)
            return false;

        outRayDir.Normalize();
        return true;
    }

    bool AimModeController::TryGetMouseRayPlaneIntersection(float planeY, engine::Vector3& outWorldPos) const
    {
        const engine::Vector2 mousePx = engine::Input::GetMousePosition();

        engine::Vector3 rayOrigin;
        engine::Vector3 rayDir;
        if (!TryBuildMouseRayFromScreen(mousePx, rayOrigin, rayDir))
            return false;

        if (std::abs(rayDir.y) <= 0.0001f)
            return false;

        const float t = (planeY - rayOrigin.y) / rayDir.y;
        if (t <= 0.0f)
            return false;

        outWorldPos = rayOrigin + rayDir * t;
        outWorldPos.y = planeY;
        return true;
    }

    void AimModeController::UpdateOnEnemyByRaycast(const engine::Vector2& mousePos, AimMode mode)
    {
        if (mode != AimMode::CombatAim)
        {
            m_isOnEnemyTarget = false;
            return;
        }

        engine::Vector3 rayOrigin;
        engine::Vector3 rayDir;
        if (!TryBuildMouseRayFromScreen(mousePos, rayOrigin, rayDir))
        {
            m_isOnEnemyTarget = false;
            return;
        }

        std::vector<engine::RaycastHit> hits;
        auto& physicsSystem = engine::SystemManager::Get().GetPhysicsSystem();
        if (!physicsSystem.RaycastAll(rayOrigin, rayDir, m_onEnemyRayMaxDistance, hits, engine::PhysicsLayer::Mask::All))
        {
            m_isOnEnemyTarget = false;
            return;
        }

        std::sort(hits.begin(), hits.end(), [](const engine::RaycastHit& a, const engine::RaycastHit& b)
            {
                return a.distance < b.distance;
            });

        bool foundValidTarget = false;
        for (const auto& hit : hits)
        {
            if (!hit.gameObject)
                continue;

            engine::GameObject* targetGO = hit.gameObject.Get();
            if (!targetGO)
                continue;

            if (targetGO->GetComponent<PlayerControllerScript>())
                continue;

            auto* damageable = targetGO->GetInterface<IDamageable>();
            if (!damageable)
                continue;

            if (auto* monster = dynamic_cast<MonsterScript*>(damageable))
            {
                if (monster->m_isFragile || monster->m_isDead)
                    continue;
            }
            else if (auto* pillar = dynamic_cast<BossPillar*>(damageable))
            {
                if (pillar->IsCrystalized())
                    continue;
            }
            else if (auto* bigProjectile = dynamic_cast<BossBigProjectile*>(damageable))
            {
                if (bigProjectile->IsCrystallized() || bigProjectile->IsDestroyed())
                    continue;
            }

            foundValidTarget = true;
            break;
        }

        m_isOnEnemyTarget = foundValidTarget;
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
        if (!m_canvas) return;

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

        if (m_cursorAimRect) m_cursorAimRect->SetAnchoredPosition(mouseRefCenter);
        if (m_cursorExecutionRect) m_cursorExecutionRect->SetAnchoredPosition(mouseRefCenter);
        if (m_cursorDefaultRect) m_cursorDefaultRect->SetAnchoredPosition(mouseRefCenter);
        if (m_cursorClickRect) m_cursorClickRect->SetAnchoredPosition(mouseRefCenter);
        if (m_cursorOnEnemyRect) m_cursorOnEnemyRect->SetAnchoredPosition(mouseRefCenter);

        // 상태에 맞는 오브젝트 활성화
        const AimCursorState desired = ComputeDesiredCursorState(mode);
        if (desired != m_cursor)
            ApplyCursorState(desired);

        // Debug
        //if (engine::Input::IsKeyPressed(engine::Keys::P))
        //{
        //    m_debugIndex++;
        //    m_debugIndex = m_debugIndex % (int)AimCursorState::Count;
        //    ApplyCursorState((AimCursorState)m_debugIndex);
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
            ImGui::SetTooltip("기본 에임 평면 높이(비이동 시). 예: 1.7");
        }
        ImGui::DragFloat("Aim Y Offset (Moving)", &m_aimYOffsetWhenMoving, 0.05f, -5.0f, 5.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("이동 중 추가되는 Y 오프셋. 예: -0.4면 1.7 -> 1.3");
        ImGui::Text("Moving: %s", m_isMoving ? "Yes" : "No");
        ImGui::DragFloat("OnEnemy Ray Max Distance", &m_onEnemyRayMaxDistance, 1.0f, 1.0f, 10000.0f);
        ImGui::Text("OnEnemy (Raycast): %s", m_isOnEnemyTarget ? "TRUE" : "false");

        ImGui::Separator();
        ImGui::Text("Cursor Pulse (AimCursor / AimCursor_OnEnemy)");
        ImGui::Checkbox("Enable Cursor Pulse", &m_enableCursorPulse);
        ImGui::DragFloat("Pulse Max Distance", &m_pulseMaxDistance, 0.1f, 0.0f, 200.0f);
        ImGui::DragFloat("Pulse Peak Time Scale", &m_pulsePeakTimeScale, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Pulse Duration Per FR", &m_pulseDurationPerFR, 0.01f, 0.1f, 5.0f);
        ImGui::Text("Pulse State: %s, %.2f", m_isPulsePlaying ? "Playing" : "Idle", m_pulseCurrent);

        ImGui::Separator();
        ImGui::Text("Execution Intro (NW/NE/SE/SW)");
        ImGui::DragFloat("Execution Intro Scale", &m_executionIntroScale, 0.01f, 1.0f, 5.0f);
        ImGui::DragFloat("Execution Intro Distance (px)", &m_executionIntroDistancePx, 0.1f, 0.0f, 200.0f);
        ImGui::DragFloat("Execution Intro Duration", &m_executionIntroDuration, 0.01f, 0.0f, 5.0f);
        ImGui::Text("Execution Intro Playing: %s", m_executionIntroPlaying ? "YES" : "NO");
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "=== Execution Cursor Debug ===");
        ImGui::DragFloat("Post-Execution Duration", &m_postExecutionDuration, 0.05f, 0.0f, 2.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("처형 완료 후 Execute 커서 유지 시간 (초)");
        
        ImGui::TextColored(m_isOnExecutionTarget ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), 
            "On Execution Target: %s", m_isOnExecutionTarget ? "TRUE" : "false");
        
        if (m_postExecutionTimer > 0.0f)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Post-Execution Timer: %.2f sec", m_postExecutionTimer);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Post-Execution Timer: %.2f sec", m_postExecutionTimer);
        }
        
        // 현재 계산된 커서 상태 표시
        AimMode effectiveMode = GetEffectiveMode();
        AimCursorState desiredState = ComputeDesiredCursorState(effectiveMode);
        const char* stateNames[] = { "Default", "Clicked", "AimIdle", "AimOnEnemy", "AimExecute" };
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Desired Cursor State: %s", stateNames[static_cast<int>(desiredState)]);

        ImGui::Separator();
        ImGui::Text("UI Cursor Objects");
        if (ImGui::DragFloat2("Cursor Size (px)", &m_cursorSize.x, 1.0f, 1.0f, 1024.0f))
        {
            if (m_cursorAimRect) m_cursorAimRect->SetSize(m_cursorSize.x, m_cursorSize.y);
            if (m_cursorExecutionRect) m_cursorExecutionRect->SetSize(m_cursorSize.x, m_cursorSize.y);
            if (m_cursorDefaultRect) m_cursorDefaultRect->SetSize(m_cursorSize.x, m_cursorSize.y);
            if (m_cursorClickRect) m_cursorClickRect->SetSize(m_cursorSize.x, m_cursorSize.y);
            if (m_cursorOnEnemyRect) m_cursorOnEnemyRect->SetSize(m_cursorSize.x, m_cursorSize.y);
        }
        ImGui::Text("Canvas (fixed): %s", kCanvasObjectName);
        ImGui::Text("Aim (fixed): %s [%s]", kCursorAimName, m_cursorAimObject ? "OK" : "MISSING");
        ImGui::Text("Execution (fixed): %s [%s]", kCursorExecutionName, m_cursorExecutionObject ? "OK" : "MISSING");
        ImGui::Text("OnEnemy (fixed): %s [%s]", kCursorOnEnemyName, m_cursorOnEnemyObject ? "OK" : "MISSING");
        ImGui::Text("Default (fixed): %s [%s]", kCursorDefaultName, m_cursorDefaultObject ? "OK" : "MISSING");
        ImGui::Text("Click (fixed): %s [%s]", kCursorClickName, m_cursorClickObject ? "OK" : "MISSING");
    }

    void AimModeController::Save(engine::json& j) const
    {        
        Object::Save(j);

        j["CursorSize"] = m_cursorSize;
        j["TargetPlaneY"] = m_targetPlaneY;
        j["AimYOffsetWhenMoving"] = m_aimYOffsetWhenMoving;
        j["OnEnemyRayMaxDistance"] = m_onEnemyRayMaxDistance;
        j["EnableCursorPulse"] = m_enableCursorPulse;
        j["PulseMaxDistance"] = m_pulseMaxDistance;
        j["PulsePeakTimeScale"] = m_pulsePeakTimeScale;
        j["PulseDurationPerFR"] = m_pulseDurationPerFR;
        j["ExecutionIntroScale"] = m_executionIntroScale;
        j["ExecutionIntroDistancePx"] = m_executionIntroDistancePx;
        j["ExecutionIntroDuration"] = m_executionIntroDuration;
        
        // 처형 커서 설정
        j["PostExecutionDuration"] = m_postExecutionDuration;
    }

    void AimModeController::Load(const engine::json& j)
    {      
        Object::Load(j);
        engine::JsonGet(j, "CursorSize", m_cursorSize);
        engine::JsonGet(j, "TargetPlaneY", m_targetPlaneY);
        engine::JsonGet(j, "AimYOffsetWhenMoving", m_aimYOffsetWhenMoving);
        engine::JsonGet(j, "OnEnemyRayMaxDistance", m_onEnemyRayMaxDistance);
        engine::JsonGet(j, "EnableCursorPulse", m_enableCursorPulse);
        engine::JsonGet(j, "PulseMaxDistance", m_pulseMaxDistance);
        engine::JsonGet(j, "PulsePeakTimeScale", m_pulsePeakTimeScale);
        engine::JsonGet(j, "PulseDurationPerFR", m_pulseDurationPerFR);
        engine::JsonGet(j, "ExecutionIntroScale", m_executionIntroScale);
        engine::JsonGet(j, "ExecutionIntroDistancePx", m_executionIntroDistancePx);
        engine::JsonGet(j, "ExecutionIntroDuration", m_executionIntroDuration);
        if (j.contains("AimYOffset") && !j.contains("AimYOffsetWhenMoving"))
            m_aimYOffsetWhenMoving = j["AimYOffset"].get<float>();

        m_pulsePeakTimeScale = std::clamp(m_pulsePeakTimeScale, 0.0f, 1.0f);
        m_pulseDurationPerFR = std::clamp(m_pulseDurationPerFR, 0.1f, 5.0f);
        m_pulseMaxDistance = std::max(0.0f, m_pulseMaxDistance);
        m_executionIntroScale = std::max(1.0f, m_executionIntroScale);
        m_executionIntroDistancePx = std::max(0.0f, m_executionIntroDistancePx);
        m_executionIntroDuration = std::max(0.0f, m_executionIntroDuration);
        
        // 처형 커서 설정
        engine::JsonGet(j, "PostExecutionDuration", m_postExecutionDuration);
    }
}