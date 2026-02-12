#include "GamePCH.h"
#include "ExecutionIndicatorManager.h"

#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/CharacterScript/Monster/MonsterPointedType.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossBigProjectile.h"
#include "Script/StageClearExecutionTarget.h"
#include "Script/ExecutionSlowScript.h"
#include "Script/CameraEffectScript.h"
#include "Script/AimModeController.h"
#include "Script/UI/UIMessageQueue.h"

#include <Core/System/Input.h>
#include <Core/System/MyTime.h>
#include <Common/Math/MathUtility.h>
#include <algorithm>
#include <limits>
#include <optional>
#include <Common/Utility/MousePicking.h>

#include <Framework/Asset/Prefab.h>
#include <Framework/Physics/CollisionTypes.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Camera.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/BoxCollider.h>
#include <Framework/Object/Component/SphereCollider.h>
#include <Framework/Object/Component/CapsuleCollider.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Engine/Framework/Object/Component/Renderer/AfterimageRenderer.h>
#include <Engine/Framework/Object/Component/Renderer/SpriteRenderer.h>

namespace game
{
    void ExecutionIndicatorManager::Awake()
    {
        // 초기화
    }

    void ExecutionIndicatorManager::Start()
    {
        // 메인 카메라 찾기
        if (auto* camGO = engine::GameObject::Find("MainCamera"))
        {
            m_mainCamera = camGO->GetComponent<engine::Camera>();
        }

        if (!m_mainCamera)
        {
            LOG_PRINT("[ExecutionIndicatorManager] WARNING: MainCamera not found!");
        }

        // 플레이어 찾기
        if (auto* playerGO = engine::GameObject::Find("Player"))
        {
            m_player = playerGO->GetComponent<PlayerControllerScript>();
        }

        if (!m_player)
        {
            LOG_PRINT("[ExecutionIndicatorManager] WARNING: Player not found!");
        }
        
        // AimModeController 찾기 (Player 오브젝트에서)
        if (m_player)
        {
            m_aimController = m_player->GetGameObject()->GetComponent<AimModeController>();
            
            if (!m_aimController)
            {
                LOG_ERROR("[ExecutionIndicatorManager] AimModeController not found on Player!");
            }
        }

        // 슬로우 효과 스크립트 찾기 (자기 자신 오브젝트에서)
        m_slowScript = GetGameObject()->GetComponent<ExecutionSlowScript>();

        if (!m_slowScript)
        {
            LOG_PRINT("[ExecutionIndicatorManager] WARNING: ExecutionSlowScript not found on this object!");
        }
        
        // 카메라 이펙트 스크립트 찾기 (MainCamera에서)
        if (auto* camGO = engine::GameObject::Find("MainCamera"))
        {
            m_cameraEffectScript = camGO->GetComponent<CameraEffectScript>();
        }

        if (auto* queueGO = engine::GameObject::Find("UIMessageQueue"))
        {
            m_uiMessageQueue = queueGO->GetComponent<UIMessageQueue>();
        }

        RefreshKillMessageKeys();

        if (auto* scene = engine::SceneManager::Get().GetScene())
        {
            m_lastSceneName = scene->GetName();
        }
    }

    void ExecutionIndicatorManager::Update()
    {
        if (!m_mainCamera) return;

        std::string currentSceneName;
        if (auto* scene = engine::SceneManager::Get().GetScene())
        {
            currentSceneName = scene->GetName();
        }
        if (!m_lastSceneName.empty() && currentSceneName != m_lastSceneName)
        {
            ResetKillMessageStats();
        }
        m_lastSceneName = currentSceneName;

        const bool isPlayerDead = (m_player && m_player->GetCurrentState() == "Dead");
        if (isPlayerDead && !m_wasPlayerDead)
        {
            ResetKillMessageStats();
        }
        m_wasPlayerDead = isPlayerDead;

        float deltaTime = engine::Time::DeltaTime();

        // 트리거 변경 후 프레임 대기 중 (이 상태에서는 다른 처리 불가)
        if (m_isWaitingForTrigger)
        {
            UpdateTriggerWait();
            return;
        }

        // 처형 애니메이션 재생 비율 대기 (제자리 애니 → 비율 도달 시 순간이동). 최소 재생 시간도 만족해야 발동
        if (m_isWaitingForExecutionAnim)
        {
            // 처형 애니 대기 중 대상이 더 이상 처형 가능하지 않으면 즉시 취소한다.
            // (예: Fragile 해제, 사망/삭제 상태 변경)
            bool isTargetStillExecutable = false;
            if (m_executingGameObject)
            {
                if (auto* monster = m_executingGameObject->GetComponent<MonsterScript>())
                {
                    isTargetStillExecutable = (monster->m_isFragile && !monster->m_isDead);
                }
                else if (auto* pillar = m_executingGameObject->GetComponent<BossPillar>())
                {
                    isTargetStillExecutable = (pillar->IsCrystalized() && !pillar->IsPendingKill());
                }
                else if (auto* projectile = m_executingGameObject->GetComponent<BossBigProjectile>())
                {
                    isTargetStillExecutable = projectile->IsCrystallized();
                }
                else if (auto* stageTarget = m_executingGameObject->GetComponent<StageClearExecutionTarget>())
                {
                    isTargetStillExecutable = stageTarget->IsExecutable();
                }
            }

            if (!isTargetStillExecutable)
            {
                CancelExecution();
                return;
            }

            float normTime = m_player ? m_player->GetExecutionAnimNormalizedTime() : -1.0f;
            float elapsed = engine::Time::UnscaledTime() - m_executionAnimWaitStartTime;
            if (normTime >= m_executionTeleportAtNormalizedTime && elapsed >= m_executionMinDuration)
            {
                m_isWaitingForExecutionAnim = false;
                PerformTeleport();
            }
        }

        // Idle 전이 대기 중
        if (m_isWaitingForIdle)
        {
            UpdateIdleWait();
            // Idle 대기 중에도 다른 처리 계속
        }

        // 몬스터 Death 타이머
        if (m_isWaitingForDeath)
        {
            UpdateDeathTimer(deltaTime);
            // Death 대기 중에도 다른 처리 계속
        }

        // 마우스 아래의 Fragile 몬스터 확인
        engine::GameObject* fragileMonster = GetFragileMonsterUnderMouse();

        if (fragileMonster)
        {
            // Fragile 몬스터 위에 마우스가 있음
            if (m_hoveredGameObject.Get() != fragileMonster)
            {
                // 새로운 몬스터로 변경
                m_hoveredGameObject = fragileMonster;
                
                // 인디케이터/라인 인스턴스 생성
                if (!m_indicatorInstance)
                {
                    CreateIndicatorInstance();
                }
            }

            // 현재 처형 중인 몬스터인지 확인
            bool isCurrentlyExecuting = (m_executingGameObject.Get() == fragileMonster);
            
            // 거리에 따른 표시 처리
            bool isInRange = IsMonsterInExecutionRange(fragileMonster);
            
            // ═══════════════════════════════════════════════════════════════
            // AimModeController에 처형 대상 위에 마우스가 있는지 즉시 알림
            // - 사거리 내 + 처형 중이 아님
            // ═══════════════════════════════════════════════════════════════
            bool isOnExecutionTarget = isInRange && !isCurrentlyExecuting;
            if (m_aimController)
            {
                m_aimController->SetOnExecutionTarget(isOnExecutionTarget);
            }
            
            // 라인: 사거리 내이고 현재 처형 중이 아닌 경우에만 표시
            if (m_player && m_lineInstance && fragileMonster->GetTransform() && !isCurrentlyExecuting && isInRange)
            {
                engine::Vector3 monsterPos = fragileMonster->GetTransform()->GetWorldPosition();
                UpdateLine(monsterPos);
                ShowLine();
            }
            else
            {
                HideLine();
            }
            
            // 반사 화살표: 빅탄이고, 사거리 내이고, 처형 중이 아닐 때만 표시
            auto* bbp = fragileMonster->GetComponent<BossBigProjectile>();
            if (bbp && bbp->IsCrystallized() && !isCurrentlyExecuting && isInRange)
            {
                UpdateReflectIndicator(fragileMonster);
                ShowReflectIndicator();
            }
            else
            {
                HideReflectIndicator();
            }
            
            // 인디케이터는 처형 사거리 내에서만 표시 (현재 처형 중인 몬스터 제외)
            if (isInRange && !isCurrentlyExecuting)
            {
                if (m_indicatorInstance && fragileMonster->GetTransform())
                {
                    ShowIndicator(fragileMonster->GetTransform());
                }
                
                // 우클릭 시 처형 시작 (사거리 내에서만)
                if (engine::Input::IsMousePressed(engine::Input::Buttons::RIGHT))
                {
                    StartExecution(fragileMonster);
                }
            }
            else
            {
                // 사거리 밖이면 인디케이터만 숨김
                HideIndicator();
            }
        }
        else
        {
            // Fragile 몬스터 위에 마우스가 없음
            if (m_hoveredGameObject)
            {
                m_hoveredGameObject = nullptr;
                HideIndicator();
                HideLine();
                HideReflectIndicator();
            }
            
            // ═══════════════════════════════════════════════════════════════
            // 마우스가 처형 대상 위에 없음을 AimModeController에 알림
            // ═══════════════════════════════════════════════════════════════
            if (m_aimController)
            {
                m_aimController->SetOnExecutionTarget(false);
            }
        }
    }

    void ExecutionIndicatorManager::CreateIndicatorInstance()
    {
        // 기존 인스턴스가 있으면 제거
        DestroyIndicatorInstance();

        // 디버그 스프라이트 비활성화:
        // - ExcutionIndicator 프리팹은 더 이상 생성하지 않는다.
        // - 판정/처형 로직은 m_indicatorInstance 없이도 유지된다.
        // engine::GameObject* indicator = engine::Prefab::Instantiate(m_indicatorPrefabName);
        // if (!indicator)
        // {
        //     LOG_PRINT("[ExecutionIndicatorManager] ERROR: Failed to instantiate prefab '{}'", m_indicatorPrefabName);
        //     return;
        // }
        // m_indicatorInstance = indicator;
        // m_indicatorTransform = indicator->GetTransform();
        // indicator->SetActive(false);

        // 라인도 생성
        CreateLineInstance();
        
        // 반사 화살표도 생성
        CreateReflectIndicatorInstance();
    }

    void ExecutionIndicatorManager::DestroyIndicatorInstance()
    {
        // 라인 먼저 제거
        DestroyLineInstance();
        
        // 반사 화살표 제거
        DestroyReflectIndicatorInstance();

        if (m_indicatorInstance)
        {
            m_indicatorInstance->Destroy();
            m_indicatorInstance = nullptr;
            m_indicatorTransform = nullptr;
        }
    }

    void ExecutionIndicatorManager::ShowIndicator(engine::Transform* targetTransform)
    {
        if (!m_indicatorInstance || !m_indicatorTransform || !targetTransform) return;

        // 몬스터 위치에 오프셋 적용
        // 인디케이터는 부모가 없으므로 로컬 위치 = 월드 위치
        engine::Vector3 position = targetTransform->GetWorldPosition();
        position.y = 0.0f;
        engine::Vector3 rPos = position + m_indicatorOffset;
        m_indicatorTransform->SetLocalPosition(rPos);

        // 활성화
        m_indicatorInstance->SetActive(true);
    }

    void ExecutionIndicatorManager::HideIndicator()
    {
        if (m_indicatorInstance)
        {
            m_indicatorInstance->SetActive(false);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 라인 관련 함수
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::CreateLineInstance()
    {
        // 기존 인스턴스가 있으면 제거
        DestroyLineInstance();

        // 프리팹 인스턴스화
        engine::GameObject* line = engine::Prefab::Instantiate(m_linePrefabName);
        if (!line)
        {
            LOG_PRINT("[ExecutionIndicatorManager] WARNING: Failed to instantiate line prefab '{}'", m_linePrefabName);
            return;
        }

        m_lineInstance = line;
        m_lineTransform = line->GetTransform();

        // 초기에는 숨김
        line->SetActive(false);
    }

    void ExecutionIndicatorManager::DestroyLineInstance()
    {
        if (m_lineInstance)
        {
            m_lineInstance->Destroy();
            m_lineInstance = nullptr;
            m_lineTransform = nullptr;
        }
    }

    void ExecutionIndicatorManager::UpdateLine(const engine::Vector3& monsterPos)
    {
        if (!m_lineTransform || !m_player) return;

        engine::Transform* playerTransform = m_player->GetTransform();
        if (!playerTransform) return;

        // 플레이어 위치
        engine::Vector3 playerPos = playerTransform->GetWorldPosition();
        playerPos.y = m_lineHeight;  // Y 높이 고정

        // 몬스터 위치 (Y 높이 고정)
        engine::Vector3 targetPos = monsterPos;
        targetPos.y = m_lineHeight;

        // 방향과 거리 계산
        engine::Vector3 direction = targetPos - playerPos;
        float distance = direction.Length();

        if (distance < 0.001f) return;  // 너무 가까우면 무시

        direction.Normalize();

        // 방향 기반 오프셋 스케일링 (Z축 → X축 선형 보간)
        float offsetMultiplier = GetDirectionalOffsetScale(direction);
        float adjustedPlayerOffset = m_linePlayerOffset * offsetMultiplier;
        float adjustedMonsterOffset = m_lineMonsterOffset * offsetMultiplier;

        // 오프셋을 적용한 실제 라인 시작/끝점 계산
        engine::Vector3 lineStart = playerPos + direction * adjustedPlayerOffset;
        engine::Vector3 lineEnd = targetPos - direction * adjustedMonsterOffset;

        // 라인 위치: 오프셋 적용된 시작점과 끝점의 중간점
        engine::Vector3 midPoint = (lineStart + lineEnd) * 0.5f;
        m_lineTransform->SetLocalPosition(midPoint);

        // 라인 회전 (로컬 X축이 direction을 향하도록)
        // 스프라이트가 X축 90도로 눕혀져 있고, 스프라이트 길이가 로컬 X축 방향
        // XZ 평면에서 방향 각도 계산 (Y축 회전만)
        float yAngle = std::atan2(direction.x, direction.z);

        // X축 90도 눕힘 + Y축 방향 회전 + Z축 90도 회전 (스프라이트 길이를 X축으로)
        // CreateFromYawPitchRoll(yaw, pitch, roll) = (Y축, X축, Z축)
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(
            yAngle,                     // Y축 회전 (방향)
            DirectX::XM_PIDIV2,         // X축 90도 (눕힘)
            DirectX::XM_PIDIV2          // Z축 90도 (스프라이트 길이를 X축으로)
        );
        m_lineTransform->SetLocalRotation(rotation);

        // ─────────────────────────────────────────────
        // 타일링 방식: 50px 텍스처를 UV Wrap으로 반복
        // ─────────────────────────────────────────────
        
        // 실제 라인 길이 = 오프셋 적용된 두 점 사이의 거리
        float lineLength = (lineEnd - lineStart).Length();
        if (lineLength < 0.001f) lineLength = 0.0f;

        // 상수 정의
        const float TEXTURE_WIDTH = 50.0f;           // 텍스처 1타일 너비 (픽셀)
        const float SPRITE_WIDTH = 50.0f;            // SpriteRenderer Width (텍스처 로드 시 자동 설정됨)
        const float ENGINE_PPU = 100.0f;             // 엔진 정책: 100 픽셀 = 1 유닛

        // 렌더링할 픽셀 크기 계산 (계단식 증가)
        // 1단계: 거리 기반 픽셀 계산
        float pixelsToRender = lineLength * m_linePixelsPerMeter;
        
        // 2단계: 단위 픽셀로 내림 (계단식 증가, 50px 단위 = 1타일 단위)
        if (m_linePixelStep > 0.001f)
        {
            pixelsToRender = std::floor(pixelsToRender / m_linePixelStep) * m_linePixelStep;
        }
        
        // 3단계: 최소값 보장
        pixelsToRender = std::max(m_lineMinPixels, pixelsToRender);

        // UV Scale: 텍스처 반복 횟수 (예: 700px / 50px = 14타일)
        // Sampler가 WRAP 모드이므로 uvScaleX > 1.0 시 텍스처가 자동 반복됨
        float uvScaleX = pixelsToRender / TEXTURE_WIDTH;

        // 엔진 렌더링 공식: finalSize = (SPRITE_WIDTH / ENGINE_PPU) × uvScale × transformScale
        // 예: (50/100) × 14 = 7.0 유닛 (쿼드 기본 크기)
        // 목표: finalSize = lineLength
        // 역계산: transformScale = lineLength / ((SPRITE_WIDTH / ENGINE_PPU) × uvScale)
        float baseSize = (SPRITE_WIDTH / ENGINE_PPU) * uvScaleX;
        float xScale = (baseSize > 0.001f) ? (lineLength / baseSize) : 1.0f;

        engine::Vector3 currentScale = m_lineTransform->GetLocalScale();
        m_lineTransform->SetLocalScale(engine::Vector3(xScale, currentScale.y, currentScale.z));

        // SpriteRenderer에 UV 타일링 설정 적용
        if (auto* renderer = m_lineInstance->GetComponent<engine::SpriteRenderer>())
        {
            renderer->SetSpriteInfo(
                engine::Vector2(0.0f, 0.0f),           // UV Offset (시작점부터)
                engine::Vector2(uvScaleX, 1.0f),       // UV Scale (X축 타일 반복, Y는 1.0)
                engine::Vector2(0.5f, 0.5f)            // Pivot (Center 유지)
            );
        }
    }

    void ExecutionIndicatorManager::ShowLine()
    {
        if (m_lineInstance)
        {
            m_lineInstance->SetActive(true);
        }
    }

    void ExecutionIndicatorManager::HideLine()
    {
        if (m_lineInstance)
        {
            m_lineInstance->SetActive(false);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 반사 화살표 관련 (빅탄 처형 전용)
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::CreateReflectIndicatorInstance()
    {
        // 기존 인스턴스가 있으면 제거
        DestroyReflectIndicatorInstance();

        // 프리팹 인스턴스화
        engine::GameObject* indicator = engine::Prefab::Instantiate(m_reflectIndicatorPrefabName);
        if (!indicator)
        {
            LOG_PRINT("[ExecutionIndicatorManager] WARNING: Failed to instantiate reflect indicator prefab '{}'", m_reflectIndicatorPrefabName);
            return;
        }

        m_reflectIndicatorInstance = indicator;
        m_reflectIndicatorTransform = indicator->GetTransform();

        // 초기에는 숨김
        indicator->SetActive(false);
    }

    void ExecutionIndicatorManager::DestroyReflectIndicatorInstance()
    {
        if (m_reflectIndicatorInstance)
        {
            m_reflectIndicatorInstance->Destroy();
            m_reflectIndicatorInstance = nullptr;
            m_reflectIndicatorTransform = nullptr;
        }
    }

    void ExecutionIndicatorManager::UpdateReflectIndicator(engine::GameObject* projectile)
    {
        if (!m_reflectIndicatorTransform || !m_player || !projectile) return;

        engine::Transform* playerTransform = m_player->GetTransform();
        engine::Transform* projectileTransform = projectile->GetTransform();
        if (!playerTransform || !projectileTransform) return;

        // 플레이어 위치
        engine::Vector3 playerPos = playerTransform->GetWorldPosition();

        // 빅탄 위치
        engine::Vector3 projectilePos = projectileTransform->GetWorldPosition();

        // 플레이어 → 빅탄 방향 계산 (Y 제거)
        engine::Vector3 direction = projectilePos - playerPos;
        direction.y = 0.0f;  // XZ 평면만
        float length = direction.Length();

        if (length < 0.001f) return;  // 너무 가까우면 무시

        direction.Normalize();

        // 방향 기반 오프셋 스케일링 적용
        float offsetScale = GetDirectionalOffsetScale(direction);
        float scaledOffset = m_reflectIndicatorOffset * offsetScale;

        // 화살표 위치: 빅탄 + (플레이어→빅탄 방향) × 스케일링된 오프셋
        engine::Vector3 arrowPos = projectilePos + direction * scaledOffset;
        arrowPos.y = m_reflectIndicatorHeight;  // Y 높이 고정
        m_reflectIndicatorTransform->SetLocalPosition(arrowPos);

        // 화살표 회전 (플레이어 → 빅탄 방향을 가리킴)
        // XZ 평면에서 방향 각도 계산 (Y축 회전)
        float yAngle = std::atan2(direction.x, direction.z);

        // X축 90도 눕힘 + Y축 방향 회전 + Z축 90도 회전 (라인과 동일)
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(
            yAngle,                     // Y축 회전 (방향)
            DirectX::XM_PIDIV2,         // X축 90도 (눕힘)
            DirectX::XM_PIDIV2          // Z축 90도 (스프라이트 길이를 X축으로)
        );
        m_reflectIndicatorTransform->SetLocalRotation(rotation);
    }

    void ExecutionIndicatorManager::ShowReflectIndicator()
    {
        if (m_reflectIndicatorInstance)
        {
            m_reflectIndicatorInstance->SetActive(true);
        }
    }

    void ExecutionIndicatorManager::HideReflectIndicator()
    {
        if (m_reflectIndicatorInstance)
        {
            m_reflectIndicatorInstance->SetActive(false);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 방향 기반 오프셋 스케일 계산
    // ═══════════════════════════════════════════════════════════════

    float ExecutionIndicatorManager::GetDirectionalOffsetScale(const engine::Vector3& direction) const
    {
        // XZ 평면에서 X/Z 비율 계산 (선형 보간)
        float absX = std::abs(direction.x);
        float absZ = std::abs(direction.z);
        float totalXZ = absX + absZ;
        
        if (totalXZ < 0.001f) return 1.0f;  // 0으로 나누기 방지
        
        float xRatio = absX / totalXZ;  // 0 (Z축 평행) ~ 1 (X축 평행)
        
        // 선형 보간: Z 스케일 → X 스케일
        // xRatio = 0 → m_offsetScaleZ
        // xRatio = 1 → m_offsetScaleX
        return m_offsetScaleZ + (m_offsetScaleX - m_offsetScaleZ) * xRatio;
    }

    // ═══════════════════════════════════════════════════════════════
    // 마우스 호버 처리
    // ═══════════════════════════════════════════════════════════════

    engine::GameObject* ExecutionIndicatorManager::GetFragileMonsterUnderMouse()
    {
        // 모든 레이어에 대해 RaycastAll 수행
        std::vector<engine::RaycastHit> allHits;
        if (!engine::MousePicking::GetAllObjectsUnderMouseDetailed(
            allHits,
            engine::PhysicsLayer::Mask::All,
            m_raycastMaxDistance))
        {
            return nullptr;
        }

        // Picking 레이어(7)인 콜라이더만 필터링
        for (const auto& hit : allHits)
        {
            // 콜라이더가 없거나 Picking 레이어가 아니면 스킵
            if (!hit.collider || hit.collider->GetLayer() != engine::PhysicsLayer::Picking)
            {
                continue;
            }

            if (!hit.gameObject)
            {
                continue;
            }

            // 히트한 오브젝트의 부모 계층에서 MonsterScript 찾기
            engine::Transform* transform = hit.gameObject->GetTransform();
            while (transform)
            {
                engine::GameObject* go = transform->GetGameObject();
                if (go)
                {
                    MonsterScript* monster = go->GetComponent<MonsterScript>();
                    if (monster)
                    {
                        // Fragile 상태인지 확인 (거리 체크는 Update에서 별도 처리)
                        if (monster->m_isFragile && !monster->m_isDead)
                        {
                            return go;
                        }
                        // MonsterScript를 찾았지만 Fragile이 아니면 다음 히트로
                        break;
                    }
                }
                transform = transform->GetParent();
            }

            transform = hit.gameObject->GetTransform();
            while (transform)
            {
                engine::GameObject* go = transform->GetGameObject();
                if (go)
                {
                    BossPillar* pillar = go->GetComponent<BossPillar>();
                    if (pillar)
                    {
                        // Fragile 상태인지 확인 (거리 체크는 Update에서 별도 처리)
                        if (pillar->IsCrystalized() && !pillar->IsPendingKill())
                        {
                            return go;
                        }
                        // MonsterScript를 찾았지만 Fragile이 아니면 다음 히트로
                        break;
                    }
                }
                transform = transform->GetParent();
            }

            transform = hit.gameObject->GetTransform();
            while (transform)
            {
                engine::GameObject* go = transform->GetGameObject();
                if (go)
                {
                    BossBigProjectile* projectile = go->GetComponent<BossBigProjectile>();
                    if (projectile)
                    {
                        // 결정화된 상태인지 확인
                        if (projectile->IsCrystallized())
                        {
                            return go;
                        }
                        break;
                    }
                }
                transform = transform->GetParent();
            }

            transform = hit.gameObject->GetTransform();
            while (transform)
            {
                engine::GameObject* go = transform->GetGameObject();
                if (go)
                {
                    StageClearExecutionTarget* stageTarget = go->GetComponent<StageClearExecutionTarget>();
                    if (stageTarget)
                    {
                        if (stageTarget->IsExecutable())
                        {
                            return go;
                        }
                        break;
                    }
                }
                transform = transform->GetParent();
            }
        }

        return nullptr;
    }

    // ═══════════════════════════════════════════════════════════════
    // 거리 계산 헬퍼
    // ═══════════════════════════════════════════════════════════════

    float ExecutionIndicatorManager::GetDistanceToMonster(engine::GameObject* target) const
    {
        if (!target || !m_player) return FLT_MAX;
        
        engine::Transform* playerTransform = m_player->GetTransform();
        engine::Transform* monsterTransform = target->GetTransform();
        
        if (!playerTransform || !monsterTransform) return FLT_MAX;
        
        engine::Vector3 playerPos = playerTransform->GetWorldPosition();
        engine::Vector3 monsterPos = monsterTransform->GetWorldPosition();
        
        return (monsterPos - playerPos).Length();
    }

    bool ExecutionIndicatorManager::IsMonsterInExecutionRange(engine::GameObject* target) const
    {
        if (!m_player) return false;
        return GetDistanceToMonster(target) <= m_player->GetExecutionRange();
    }

    void ExecutionIndicatorManager::RefreshKillMessageKeys()
    {
        if (!m_uiMessageQueue)
        {
            if (auto* queueGO = engine::GameObject::Find("UIMessageQueue"))
            {
                m_uiMessageQueue = queueGO->GetComponent<UIMessageQueue>();
            }
        }
        if (!m_uiMessageQueue)
        {
            m_killMessageKeys.clear();
            return;
        }

        std::vector<std::string> keys;
        m_uiMessageQueue->CollectCatalogKeys(UIMessageChannel::Kill, "Kill_", keys);
        m_killMessageKeys = std::move(keys);

        for (const auto& key : m_killMessageKeys)
        {
            if (m_killMessageUseCount.find(key) == m_killMessageUseCount.end())
            {
                m_killMessageUseCount[key] = 0;
            }
        }

        for (auto it = m_killMessageUseCount.begin(); it != m_killMessageUseCount.end(); )
        {
            if (std::find(m_killMessageKeys.begin(), m_killMessageKeys.end(), it->first) == m_killMessageKeys.end())
            {
                it = m_killMessageUseCount.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void ExecutionIndicatorManager::ResetKillMessageStats()
    {
        m_killMessageUseCount.clear();
        m_lastKillMessageKey.clear();
        RefreshKillMessageKeys();
    }

    std::string ExecutionIndicatorManager::BuildBalancedKillMessageKey()
    {
        RefreshKillMessageKeys();
        if (m_killMessageKeys.empty())
        {
            return "";
        }

        int minCount = std::numeric_limits<int>::max();
        for (const auto& key : m_killMessageKeys)
        {
            const int count = m_killMessageUseCount[key];
            if (count < minCount)
            {
                minCount = count;
            }
        }

        std::vector<std::string> candidates;
        for (const auto& key : m_killMessageKeys)
        {
            if (m_killMessageUseCount[key] == minCount)
            {
                candidates.push_back(key);
            }
        }

        if (candidates.size() > 1 && !m_lastKillMessageKey.empty())
        {
            candidates.erase(
                std::remove(candidates.begin(), candidates.end(), m_lastKillMessageKey),
                candidates.end());
        }
        if (candidates.empty())
        {
            candidates = m_killMessageKeys;
        }

        const size_t idx = engine::Random::Int<size_t>(0, candidates.size() - 1);
        const std::string selectedKey = candidates[idx];
        ++m_killMessageUseCount[selectedKey];
        m_lastKillMessageKey = selectedKey;
        return selectedKey;
    }

    void ExecutionIndicatorManager::PushRandomKillMessage()
    {
        const std::string key = BuildBalancedKillMessageKey();
        if (key.empty() || !m_uiMessageQueue)
        {
            return;
        }

        m_uiMessageQueue->PushMessageKey(key);
    }

    void ExecutionIndicatorManager::StartExecution(engine::GameObject* target)
    {
        if (!target || m_isWaitingForTrigger || !m_player) return;

        // 우클릭 처형 요청 시점부터 즉시 처형 무적 시작
        m_player->StartExecutionInvincibility();

        // 처형 요청이 시작되는 즉시 Execute 커서를 고정한다.
        if (m_aimController)
        {
            m_aimController->SetExecutionInProgress(true);
        }

        // ─────────────────────────────────────────────
        // 연속 처형: 기존 처형 진행 중이면 정리 후 새 처형 시작
        // ─────────────────────────────────────────────
        if (m_isWaitingForDeath || m_isWaitingForIdle || m_isWaitingForExecutionAnim)
        {
            // 이전 몬스터 즉시 Death 처리
            if (m_executingGameObject && m_isWaitingForDeath)
            {
                TriggerMonsterDeath();
            }
            
            // 상태 리셋
            m_isWaitingForDeath = false;
            m_isWaitingForIdle = false;
            m_isWaitingForExecutionAnim = false;
            m_executionAnimWaitStartTime = 0.0f;
            m_deathTimer = 0.0f;
        }

        m_executingGameObject = target;

        // ─────────────────────────────────────────────
        // 1. 몬스터 콜라이더를 트리거로 즉시 변경
        //    (Execution 전이는 트리거 확인 후에 진행)
        // ─────────────────────────────────────────────
        SetMonsterColliderTrigger(target, true);

        // ─────────────────────────────────────────────
        // 2. 프레임 대기 시작 (물리 적용 확인용)
        // ─────────────────────────────────────────────
        m_isWaitingForTrigger = true;
        m_triggerWaitFrames = 0;
    }

    // ═══════════════════════════════════════════════════════════════
    // 콜라이더 트리거 설정
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::SetMonsterColliderTrigger(engine::GameObject* monster, bool isTrigger)
    {
        if (!monster) return;

        if (auto* boxCollider = monster->GetComponent<engine::BoxCollider>())
        {
            boxCollider->SetIsTrigger(isTrigger);
        }
        if (auto* sphereCollider = monster->GetComponent<engine::SphereCollider>())
        {
            sphereCollider->SetIsTrigger(isTrigger);
        }
        if (auto* capsuleCollider = monster->GetComponent<engine::CapsuleCollider>())
        {
            capsuleCollider->SetIsTrigger(isTrigger);
        }
    }

    bool ExecutionIndicatorManager::IsMonsterColliderTrigger(engine::GameObject* monster) const
    {
        if (!monster) return false;

        // 하나라도 트리거로 변경되었는지 확인
        if (auto* boxCollider = monster->GetComponent<engine::BoxCollider>())
        {
            if (boxCollider->IsTrigger()) return true;
        }
        if (auto* sphereCollider = monster->GetComponent<engine::SphereCollider>())
        {
            if (sphereCollider->IsTrigger()) return true;
        }
        if (auto* capsuleCollider = monster->GetComponent<engine::CapsuleCollider>())
        {
            if (capsuleCollider->IsTrigger()) return true;
        }

        return false;
    }

    bool ExecutionIndicatorManager::IsPathClearForTeleport() const
    {
        // TODO: 향후 구현 - 근처 충돌 가능한 콜라이더 검사
        // 현재는 항상 true 반환 (즉시 이동 허용)
        return true;
    }

    void ExecutionIndicatorManager::CancelExecution()
    {
        // 트리거 확인 실패 → 처형 취소
        // 콜라이더 복원
        if (m_executingGameObject)
        {
            SetMonsterColliderTrigger(m_executingGameObject.Get(), false);
        }

        // 상태 초기화
        m_isWaitingForTrigger = false;
        m_isWaitingForExecutionAnim = false;
        m_isWaitingForIdle = false;
        m_isWaitingForDeath = false;
        m_executionAnimWaitStartTime = 0.0f;
        m_idleWaitFrames = 0;
        m_deathTimer = 0.0f;
        m_triggerWaitFrames = 0;

        // 처형 도중 취소된 경우 플레이어를 Execution 상태에서 정상 복귀시킨다.
        if (m_player)
        {
            // 처형 취소(예: Fragile 해제 포함) 시 처형 무적도 즉시 종료
            m_player->EndExecutionInvincibility();

            engine::LogicFSM* playerFSM = m_player->GetGameObject()->GetComponent<engine::LogicFSM>();
            if (playerFSM && playerFSM->GetCurrentState() == "Execution")
            {
                playerFSM->SetTrigger("ExecutionComplete");
            }
        }

        m_executingGameObject = nullptr;

        if (m_aimController)
        {
            m_aimController->SetExecutionInProgress(false);
        }

        // 인디케이터 매니저는 계속 동작 (호버/라인 표시 등)
    }

    // ═══════════════════════════════════════════════════════════════
    // Idle 전이 대기
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::UpdateIdleWait()
    {
        if (!m_isWaitingForIdle) return;

        m_idleWaitFrames++;

        // 1프레임 대기 후 Idle 전이
        if (m_idleWaitFrames >= 1)
        {
            m_isWaitingForIdle = false;

            if (m_player)
            {
                engine::LogicFSM* playerFSM = m_player->GetGameObject()->GetComponent<engine::LogicFSM>();
                if (playerFSM)
                {
                    playerFSM->SetTrigger("ExecutionComplete");
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 몬스터 Death 타이머
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::UpdateDeathTimer(float deltaTime)
    {
        if (!m_isWaitingForDeath) return;

        m_deathTimer += deltaTime;

        if (m_deathTimer >= m_monsterDeathDelay)
        {
            m_isWaitingForDeath = false;
            TriggerMonsterDeath();
        }
    }

    void ExecutionIndicatorManager::TriggerMonsterDeath()
    {
        if (!m_executingGameObject) return;

        if (auto comp = m_executingGameObject->GetComponent<MonsterScript>())
        {
            const bool isPointedType = (m_executingGameObject->GetComponent<MonsterPointedType>() != nullptr);
            comp->TriggerDeath();
            if (isPointedType)
            {
                PushRandomKillMessage();
            }
        }
        else if (auto comp = m_executingGameObject->GetComponent<BossPillar>())
        {
            comp->Execute();
        }
        else if (auto comp = m_executingGameObject->GetComponent<StageClearExecutionTarget>())
        {
            comp->Execute();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 트리거 변경 후 프레임 대기
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::UpdateTriggerWait()
    {
        if (!m_isWaitingForTrigger) return;

        // 대상이 대기 중에 파괴/무효화되면 즉시 취소하여 상태 고착을 방지한다.
        if (!m_executingGameObject)
        {
            CancelExecution();
            return;
        }

        m_triggerWaitFrames++;

        // 필요한 프레임 수 대기 완료
        if (m_triggerWaitFrames >= m_triggerWaitFramesRequired)
        {
            // 트리거 변경 확인
            if (!IsMonsterColliderTrigger(m_executingGameObject.Get()))
            {
                // 트리거 변경 실패 → 처형 취소
                CancelExecution();
                return;
            }

            // 향후: 경로 확인 (근처 충돌 가능 콜라이더 검사)
            if (!IsPathClearForTeleport())
            {
                // 경로가 안전하지 않으면 계속 대기 (향후 구현)
                return;
            }

            m_isWaitingForTrigger = false;

            // ─────────────────────────────────────────────
            // 트리거 확인 성공 → 처형 진행 (제자리 애니 재생 후 비율 도달 시 순간이동)
            // 슬로우는 PerformTeleport()에서 순간이동·처형 시점에 시작 (애니 재생 중엔 노멀 속도)
            // ─────────────────────────────────────────────

            // 1. 플레이어 Execution 상태 전이 (제자리에서 처형 애니 재생)
            engine::LogicFSM* playerFSM = m_player->GetGameObject()->GetComponent<engine::LogicFSM>();
            if (playerFSM)
            {
                playerFSM->SetTrigger("ExecuteMonster");
            }

            // 2. 처형 애니 재생 비율 도달까지 대기, 도달 시 PerformTeleport()에서 순간이동·슬로우·처형·상태 종료
            m_isWaitingForExecutionAnim = true;
            m_executionAnimWaitStartTime = engine::Time::UnscaledTime();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 순간이동 (한 번에 몬스터 위치로)
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::PerformTeleport()
    {
        if (!m_executingGameObject || !m_player) return;

        engine::Transform* monsterTransform = m_executingGameObject->GetTransform();
        engine::Vector3 monsterPos = monsterTransform ? monsterTransform->GetWorldPosition() : engine::Vector3::Zero;

        // 슬로우 모션: 순간이동·처형이 실행되는 이 순간부터 시작 (애니 재생 중엔 노멀 속도)
        if (m_slowScript)
        {
            m_slowScript->StartSlowMotion();
        }

        // ─────────────────────────────────────────────
        // 텔레포트 잔상: 플레이어 메시 현재 위치 → 몬스터 위치 구간에 Afterimage 슬라이스 채움 (순간이동 직전)
        // ─────────────────────────────────────────────
        if (monsterTransform && m_teleportAfterimageNumSlices > 0)
        {
            if (engine::AfterimageRenderer* afterimage = m_player->GetAfterImage())
            {
                engine::Matrix fromWorld = afterimage->GetGameObject()->GetTransform()->GetWorld();
                engine::Matrix toWorld = fromWorld;
                toWorld._41 = monsterPos.x;
                toWorld._42 = monsterPos.y;
                toWorld._43 = monsterPos.z;
                afterimage->ClearSlices();
                std::optional<float> gradOverride = m_teleportAfterimageUseDefaultGradient
                    ? std::nullopt
                    : std::optional<float>(m_teleportAfterimageTrailGradient);
                afterimage->RecordTeleportPath(fromWorld, toWorld, m_teleportAfterimageNumSlices, gradOverride);
            }
        }

        // ─────────────────────────────────────────────
        // 라인 즉시 숨김
        // ─────────────────────────────────────────────
        HideLine();
        
        // 호버 인디케이터 숨김 (처형 중인 몬스터)
        HideIndicator();

        // ─────────────────────────────────────────────
        // 보스 투사체 특수 처리
        // ─────────────────────────────────────────────
        if (auto comp = m_executingGameObject->GetComponent<BossBigProjectile>())
        {
            comp->Execute();
        }

        // ─────────────────────────────────────────────
        // 처형 이펙트 인스턴시에이트 (몬스터 위치)
        // ─────────────────────────────────────────────
        if (monsterTransform)
        {
            // 이펙트 프리팹 생성
            // 디버그 스프라이트 비활성화:
            // - ExcutionEffectSprite 프리팹은 더 이상 생성하지 않는다.
            // engine::GameObject* effect = engine::Prefab::Instantiate(m_effectPrefabName);
            // if (effect)
            // {
            //     // 몬스터 위치 + 인디케이터 오프셋에 배치
            //     if (auto* effectTransform = effect->GetTransform())
            //     {
            //         effectTransform->SetLocalPosition(monsterPos + m_indicatorOffset);
            //     }
            //     
            //     // 이펙트 설정 전달
            //     if (auto* effectScript = effect->GetComponent<ExecutionEffectScript>())
            //     {
            //         effectScript->SetDuration(m_effectDuration);
            //         effectScript->SetScaleMultiplier(m_effectScaleMultiplier);
            //     }
            // }
            
            // 플레이어 순간이동 (Y=0 강제)
            engine::Vector3 teleportPos = monsterPos;
            teleportPos.y = 0.0f;
            
            engine::Rigidbody* rigidbody = m_player->GetGameObject()->GetComponent<engine::Rigidbody>();
            if (rigidbody && rigidbody->IsDynamic())
            {
                rigidbody->ForceSetPosition(teleportPos, true);
            }
            else if (m_player->GetTransform())
            {
                m_player->GetTransform()->SetLocalPosition(teleportPos);
            }
            
            // 카메라 이펙트 시작 (텔레포트 직후)
            if (m_cameraEffectScript)
            {
                m_cameraEffectScript->StartZoomEffect(m_cameraEffectScript->GetDefaultDuration());
            }
        }

        // ─────────────────────────────────────────────
        // 1프레임 후 Idle 전이 시작
        // ─────────────────────────────────────────────
        m_isWaitingForIdle = true;
        m_idleWaitFrames = 0;

        // ─────────────────────────────────────────────
        // 처형 대상은 텔레포트 직후 즉시 처형 처리한다.
        // (기존 딜레이로 인해 1프레임 튀어 보이는 현상 방지)
        // ─────────────────────────────────────────────
        m_isWaitingForDeath = false;
        m_deathTimer = 0.0f;
        TriggerMonsterDeath();
    }


    void ExecutionIndicatorManager::OnGui()
    {
        ImGui::TextDisabled("Debug sprite indicator/effect prefab is disabled.");
        ImGui::DragFloat("Raycast Distance", &m_raycastMaxDistance, 10.0f, 100.0f, 10000.0f);

        ImGui::Separator();
        ImGui::Text("Line Settings:");
        ImGui::InputText("Line Prefab", &m_linePrefabName);
        ImGui::DragFloat("Line Player Offset", &m_linePlayerOffset, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Line Monster Offset", &m_lineMonsterOffset, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Line Base Length", &m_lineBaseLength, 0.1f, 0.01f, 10.0f);
        ImGui::DragFloat("Line Height", &m_lineHeight, 0.1f, 0.0f, 10.0f);
        
        if (ImGui::DragFloat("Line Pixels Per Meter", &m_linePixelsPerMeter, 1.0f, 10.0f, 200.0f, "%.1f"))
        {
            m_linePixelsPerMeter = std::clamp(m_linePixelsPerMeter, 10.0f, 200.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("1m당 텍스처 픽셀 수 (점선 밀도 조절). 값이 클수록 점선이 빽빽함");
        }
        
        if (ImGui::DragFloat("Line Min Pixels", &m_lineMinPixels, 1.0f, 1.0f, 500.0f, "%.1f"))
        {
            m_lineMinPixels = std::clamp(m_lineMinPixels, 1.0f, 500.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("최소 드로우 픽셀 크기 (최소 라인 길이)");
        }
        
        if (ImGui::DragFloat("Line Pixel Step", &m_linePixelStep, 1.0f, 1.0f, 200.0f, "%.1f"))
        {
            m_linePixelStep = std::clamp(m_linePixelStep, 1.0f, 200.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("증가 단위 픽셀 (계단식 증가). 예: 50이면 50px 단위로만 증가");
        }

        ImGui::Separator();
        ImGui::Text("Reflect Indicator Settings (Big Projectile only):");
        ImGui::InputText("Reflect Indicator Prefab", &m_reflectIndicatorPrefabName);
        
        if (ImGui::DragFloat("Reflect Indicator Height", &m_reflectIndicatorHeight, 0.1f, 0.0f, 10.0f, "%.2f"))
        {
            m_reflectIndicatorHeight = std::clamp(m_reflectIndicatorHeight, 0.0f, 10.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("화살표의 Y 높이 (고정값)");
        }
        
        if (ImGui::DragFloat("Reflect Indicator Offset", &m_reflectIndicatorOffset, 0.1f, -10.0f, 20.0f, "%.2f"))
        {
            m_reflectIndicatorOffset = std::clamp(m_reflectIndicatorOffset, -10.0f, 20.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("빅탄 중점에서 반사 방향으로의 오프셋 거리 (m). 음수: 플레이어 쪽");
        }

        ImGui::Separator();
        ImGui::Text("Directional Offset Scaling:");
        if (ImGui::DragFloat("Offset Scale Z-axis", &m_offsetScaleZ, 0.01f, 0.0f, 2.0f, "%.2f"))
        {
            m_offsetScaleZ = std::clamp(m_offsetScaleZ, 0.0f, 2.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Z축 평행 방향일 때 오프셋 배율 (0~2). 라인과 반사 화살표에 모두 적용");
        }
        
        if (ImGui::DragFloat("Offset Scale X-axis", &m_offsetScaleX, 0.01f, 0.0f, 2.0f, "%.2f"))
        {
            m_offsetScaleX = std::clamp(m_offsetScaleX, 0.0f, 2.0f);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("X축 평행 방향일 때 오프셋 배율 (0~2). 라인과 반사 화살표에 모두 적용");
        }

        ImGui::Separator();
        ImGui::Text("Execution Effect Settings:");
        ImGui::TextDisabled("Execution debug sprite effect is disabled.");
        ImGui::DragFloat("Monster Death Delay", &m_monsterDeathDelay, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Delay after teleport before monster death");
        }

        ImGui::Separator();
        ImGui::Text("Execution Timing (when to teleport):");
        ImGui::DragFloat("Teleport At Normalized Time (0~1)", &m_executionTeleportAtNormalizedTime, 0.01f, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("처형 애니 재생 비율. 이 값에 도달하면 순간이동·처형 실행 (0.5 = 50%%, 0.3 = 빨리, 0.7 = 늦게)");
        }
        ImGui::DragFloat("Execution Min Duration (sec)", &m_executionMinDuration, 0.05f, 0.0f, 2.0f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("처형 애니 최소 재생 시간. 이 시간이 지나야 비율 조건과 함께 처형 발동 (가까운 적이어도 애니가 잠깐 보이게)");
        int teleportSlices = static_cast<int>(m_teleportAfterimageNumSlices);
        ImGui::DragInt("Teleport Afterimage Slices", &teleportSlices, 1, 0, 64);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("처형 순간이동 시 출발→도착 구간에 넣을 잔상(Afterimage) 슬라이스 수. 0이면 잔상 없음");
        m_teleportAfterimageNumSlices = static_cast<size_t>(std::max(0, teleportSlices));
        ImGui::Checkbox("Teleport Afterimage Use Default Gradient", &m_teleportAfterimageUseDefaultGradient);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("체크 시 AfterimageRenderer 기본 trail gradient 사용, 해제 시 아래 값으로 오버라이드");
        if (!m_teleportAfterimageUseDefaultGradient)
        {
            ImGui::DragFloat("Teleport Afterimage Trail Gradient", &m_teleportAfterimageTrailGradient, 0.01f, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("1에 가까울수록 구간 전체 비슷한 알파, 작을수록 앞쪽이 빨리 흐려짐 (예: 0.97)");
        }

        ImGui::DragInt("Trigger Wait Frames", &m_triggerWaitFramesRequired, 1, 1, 10);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Frames to wait after collider trigger change before teleport");
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "=== DEBUG: Execution Cursor ===");
        
        // AimController 연결 상태
        if (m_aimController)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "AimController: FOUND");
            
            // 현재 처형 대상 감지 상태
            engine::GameObject* fragileMonster = GetFragileMonsterUnderMouse();
            if (fragileMonster)
            {
                bool isCurrentlyExecuting = (m_executingGameObject.Get() == fragileMonster);
                bool isInRange = IsMonsterInExecutionRange(fragileMonster);
                bool shouldShowExecute = isInRange && !isCurrentlyExecuting;
                
                ImGui::TextColored(ImVec4(0, 1, 1, 1), "Fragile Monster Under Mouse: %s", fragileMonster->GetName().c_str());
                ImGui::Text("  In Range: %s", isInRange ? "YES" : "NO");
                ImGui::Text("  Currently Executing: %s", isCurrentlyExecuting ? "YES" : "NO");
                ImGui::TextColored(shouldShowExecute ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), 
                    "  Should Show Execute: %s", shouldShowExecute ? "YES" : "NO");
            }
            else
            {
                ImGui::Text("Fragile Monster Under Mouse: None");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "AimController: NOT FOUND!");
        }

        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Hovered Monster: %s", m_hoveredGameObject ? "Yes" : "No");
        ImGui::Text("Executing Monster: %s", m_executingGameObject ? "Yes" : "No");
        ImGui::Text("Slow Script Found: %s", m_slowScript ? "Yes" : "No");
        ImGui::Text("Camera Effect Script Found: %s", m_cameraEffectScript ? "Yes" : "No");
        ImGui::Text("Player Found: %s", m_player ? "Yes" : "No");
        ImGui::Text("Line Instance: %s", m_lineInstance ? "Yes" : "No");
        
        ImGui::Separator();
        ImGui::Text("Execution State:");
        ImGui::Text("Waiting for Trigger: %s", m_isWaitingForTrigger ? "Yes" : "No");
        ImGui::Text("Waiting for Execution Anim: %s", m_isWaitingForExecutionAnim ? "Yes" : "No");
        ImGui::Text("Waiting for Idle: %s", m_isWaitingForIdle ? "Yes" : "No");
        ImGui::Text("Waiting for Death: %s", m_isWaitingForDeath ? "Yes" : "No");
        
        if (m_isWaitingForTrigger)
        {
            ImGui::Text("  Trigger Wait: %d / %d frames", m_triggerWaitFrames, m_triggerWaitFramesRequired);
        }
        if (m_isWaitingForDeath)
        {
            ImGui::Text("  Death Timer: %.3f / %.3f sec", m_deathTimer, m_monsterDeathDelay);
        }
    }

    void ExecutionIndicatorManager::Save(engine::json& j) const
    {
        Object::Save(j);
        j["IndicatorPrefabName"] = m_indicatorPrefabName;
        j["RaycastMaxDistance"] = m_raycastMaxDistance;
        j["IndicatorOffset"] = m_indicatorOffset;

        // 라인 설정
        j["LinePrefabName"] = m_linePrefabName;
        j["LinePlayerOffset"] = m_linePlayerOffset;
        j["LineMonsterOffset"] = m_lineMonsterOffset;
        j["LineBaseLength"] = m_lineBaseLength;
        j["LineHeight"] = m_lineHeight;
        j["LinePixelsPerMeter"] = m_linePixelsPerMeter;
        j["LineMinPixels"] = m_lineMinPixels;
        j["LinePixelStep"] = m_linePixelStep;
        
        // 반사 화살표 설정
        j["ReflectIndicatorPrefabName"] = m_reflectIndicatorPrefabName;
        j["ReflectIndicatorHeight"] = m_reflectIndicatorHeight;
        j["ReflectIndicatorOffset"] = m_reflectIndicatorOffset;
        
        // 방향 기반 오프셋 스케일링
        j["OffsetScaleZ"] = m_offsetScaleZ;
        j["OffsetScaleX"] = m_offsetScaleX;

        // 처형 이펙트 설정
        j["EffectPrefabName"] = m_effectPrefabName;
        j["EffectDuration"] = m_effectDuration;
        j["EffectScaleMultiplier"] = m_effectScaleMultiplier;
        j["MonsterDeathDelay"] = m_monsterDeathDelay;
        j["ExecutionTeleportAtNormalizedTime"] = m_executionTeleportAtNormalizedTime;
        j["ExecutionMinDuration"] = m_executionMinDuration;
        j["TeleportAfterimageNumSlices"] = static_cast<int>(m_teleportAfterimageNumSlices);
        j["TeleportAfterimageUseDefaultGradient"] = m_teleportAfterimageUseDefaultGradient;
        j["TeleportAfterimageTrailGradient"] = m_teleportAfterimageTrailGradient;
        j["TriggerWaitFramesRequired"] = m_triggerWaitFramesRequired;
    }

    void ExecutionIndicatorManager::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "IndicatorPrefabName", m_indicatorPrefabName);
        engine::JsonGet(j, "RaycastMaxDistance", m_raycastMaxDistance);
        engine::JsonGet(j, "IndicatorOffset", m_indicatorOffset);

        // 라인 설정
        engine::JsonGet(j, "LinePrefabName", m_linePrefabName);
        engine::JsonGet(j, "LinePlayerOffset", m_linePlayerOffset);
        engine::JsonGet(j, "LineMonsterOffset", m_lineMonsterOffset);
        engine::JsonGet(j, "LineBaseLength", m_lineBaseLength);
        engine::JsonGet(j, "LineHeight", m_lineHeight);
        
        if (j.contains("LinePixelsPerMeter"))
            m_linePixelsPerMeter = std::clamp(j["LinePixelsPerMeter"].get<float>(), 10.0f, 200.0f);
        if (j.contains("LineMinPixels"))
            m_lineMinPixels = j["LineMinPixels"].get<float>();
        if (j.contains("LinePixelStep"))
            m_linePixelStep = j["LinePixelStep"].get<float>();
        
        // 반사 화살표 설정
        engine::JsonGet(j, "ReflectIndicatorPrefabName", m_reflectIndicatorPrefabName);
        engine::JsonGet(j, "ReflectIndicatorHeight", m_reflectIndicatorHeight);
        engine::JsonGet(j, "ReflectIndicatorOffset", m_reflectIndicatorOffset);
        
        // 방향 기반 오프셋 스케일링
        engine::JsonGet(j, "OffsetScaleZ", m_offsetScaleZ);
        engine::JsonGet(j, "OffsetScaleX", m_offsetScaleX);

        // 처형 이펙트 설정
        engine::JsonGet(j, "EffectPrefabName", m_effectPrefabName);
        engine::JsonGet(j, "EffectDuration", m_effectDuration);
        engine::JsonGet(j, "EffectScaleMultiplier", m_effectScaleMultiplier);
        engine::JsonGet(j, "MonsterDeathDelay", m_monsterDeathDelay);
        engine::JsonGet(j, "ExecutionTeleportAtNormalizedTime", m_executionTeleportAtNormalizedTime);
        engine::JsonGet(j, "ExecutionMinDuration", m_executionMinDuration);
        engine::JsonGet(j, "TeleportAfterimageUseDefaultGradient", m_teleportAfterimageUseDefaultGradient);
        engine::JsonGet(j, "TeleportAfterimageTrailGradient", m_teleportAfterimageTrailGradient);
        if (j.contains("TeleportAfterimageNumSlices"))
        {
            int n = j["TeleportAfterimageNumSlices"].get<int>();
            m_teleportAfterimageNumSlices = static_cast<size_t>(std::max(0, n));
        }
        engine::JsonGet(j, "TriggerWaitFramesRequired", m_triggerWaitFramesRequired);
    }
}
