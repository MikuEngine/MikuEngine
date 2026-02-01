#include "GamePCH.h"
#include "ExecutionIndicatorManager.h"

#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossProjectile.h"
#include "Script/ExecutionSlowScript.h"
#include "Script/ExecutionEffectScript.h"
#include "Script/CameraEffectScript.h"

#include <Core/System/Input.h>
#include <Core/System/MyTime.h>
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
    }

    void ExecutionIndicatorManager::Update()
    {
        if (!m_mainCamera) return;

        float deltaTime = engine::Time::DeltaTime();

        // 트리거 변경 후 프레임 대기 중 (이 상태에서는 다른 처리 불가)
        if (m_isWaitingForTrigger)
        {
            UpdateTriggerWait();
            return;
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
            
            // 라인: 현재 처형 중인 몬스터가 아닌 경우에만 표시
            if (m_player && m_lineInstance && fragileMonster->GetTransform() && !isCurrentlyExecuting)
            {
                engine::Vector3 monsterPos = fragileMonster->GetTransform()->GetWorldPosition();
                UpdateLine(monsterPos);
                ShowLine();
            }
            else if (isCurrentlyExecuting)
            {
                HideLine();
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
            }
        }
    }

    void ExecutionIndicatorManager::CreateIndicatorInstance()
    {
        // 기존 인스턴스가 있으면 제거
        DestroyIndicatorInstance();

        // 프리팹 인스턴스화
        engine::GameObject* indicator = engine::Prefab::Instantiate(m_indicatorPrefabName);
        if (!indicator)
        {
            LOG_PRINT("[ExecutionIndicatorManager] ERROR: Failed to instantiate prefab '{}'", m_indicatorPrefabName);
            return;
        }

        m_indicatorInstance = indicator;
        m_indicatorTransform = indicator->GetTransform();

        // 초기에는 숨김
        indicator->SetActive(false);

        // 라인도 생성
        CreateLineInstance();
    }

    void ExecutionIndicatorManager::DestroyIndicatorInstance()
    {
        // 라인 먼저 제거
        DestroyLineInstance();

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
        engine::Vector3 position = targetTransform->GetWorldPosition() + m_indicatorOffset;
        m_indicatorTransform->SetLocalPosition(position);

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

        // 45도 탑뷰 시점 보정: X축 방향일 때 오프셋 100%, Z축 방향일 때 90%
        // 배율 = 0.9 + 0.1 * |direction.x|
        float offsetMultiplier = 0.9f + 0.1f * std::abs(direction.x);
        float adjustedPlayerOffset = m_linePlayerOffset * offsetMultiplier;
        float adjustedMonsterOffset = m_lineMonsterOffset * offsetMultiplier;

        // 라인 위치: 플레이어와 몬스터의 중간점 (피벗이 중앙이므로)
        engine::Vector3 midPoint = (playerPos + targetPos) * 0.5f;
        m_lineTransform->SetLocalPosition(midPoint);

        // 라인 회전 (로컬 Y축이 direction을 향하도록)
        // 스프라이트가 X축 90도로 눕혀져 있어서 로컬 Y축이 Forward
        // XZ 평면에서 방향 각도 계산 (Y축 회전만)
        float yAngle = std::atan2(direction.x, direction.z);

        // X축 90도 눕힘 + Y축 방향 회전
        // CreateFromYawPitchRoll(yaw, pitch, roll) = (Y축, X축, Z축)
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(
            yAngle,                     // Y축 회전 (방향)
            DirectX::XM_PIDIV2,         // X축 90도 (눕힘)
            0.0f                        // Z축 회전 없음
        );
        m_lineTransform->SetLocalRotation(rotation);

        // 라인 스케일 (Y축만 조정 - 양방향으로 늘어나므로)
        // 실제 라인 길이 = 거리 - 보정된 양쪽 오프셋
        float lineLength = distance - adjustedPlayerOffset - adjustedMonsterOffset;
        if (lineLength < 0.0f) lineLength = 0.0f;

        float yScale = lineLength / m_lineBaseLength;
        engine::Vector3 currentScale = m_lineTransform->GetLocalScale();
        m_lineTransform->SetLocalScale(engine::Vector3(currentScale.x, yScale, currentScale.z));
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
                    BossProjectile* projectile = go->GetComponent<BossProjectile>();
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

    void ExecutionIndicatorManager::StartExecution(engine::GameObject* target)
    {
        if (!target || m_isWaitingForTrigger || !m_player) return;

        // ─────────────────────────────────────────────
        // 연속 처형: 기존 처형 진행 중이면 정리 후 새 처형 시작
        // ─────────────────────────────────────────────
        if (m_isWaitingForDeath || m_isWaitingForIdle)
        {
            // 이전 몬스터 즉시 Death 처리
            if (m_executingGameObject && m_isWaitingForDeath)
            {
                TriggerMonsterDeath();
            }
            
            // 상태 리셋
            m_isWaitingForDeath = false;
            m_isWaitingForIdle = false;
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

        // 상태 초기화 (플레이어/몬스터 상태 유지)
        m_isWaitingForTrigger = false;
        m_triggerWaitFrames = 0;
        m_executingGameObject = nullptr;

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
            comp->TriggerDeath();
        }
        else if (auto comp = m_executingGameObject->GetComponent<BossPillar>())
        {
            comp->Execute();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 트리거 변경 후 프레임 대기
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::UpdateTriggerWait()
    {
        if (!m_isWaitingForTrigger || !m_executingGameObject) return;

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
            // 트리거 확인 성공 → 처형 진행
            // ─────────────────────────────────────────────

            // 1. 슬로우 효과 시작
            if (m_slowScript)
            {
                m_slowScript->StartSlowMotion();
            }

            // 2. 플레이어 Execution 스테이트로 전이
            engine::LogicFSM* playerFSM = m_player->GetGameObject()->GetComponent<engine::LogicFSM>();
            if (playerFSM)
            {
                playerFSM->SetTrigger("ExecuteMonster");
            }

            // 3. 텔레포트 실행
            PerformTeleport();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 순간이동 (한 번에 몬스터 위치로)
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::PerformTeleport()
    {
        if (!m_executingGameObject || !m_player) return;

        // ─────────────────────────────────────────────
        // 라인 즉시 숨김
        // ─────────────────────────────────────────────
        HideLine();
        
        // 호버 인디케이터 숨김 (처형 중인 몬스터)
        HideIndicator();

        // ─────────────────────────────────────────────
        // 보스 투사체 특수 처리
        // ─────────────────────────────────────────────
        if (auto comp = m_executingGameObject->GetComponent<BossProjectile>())
        {
            comp->Execute();
        }

        // ─────────────────────────────────────────────
        // 처형 이펙트 인스턴시에이트 (몬스터 위치)
        // ─────────────────────────────────────────────
        engine::Transform* monsterTransform = m_executingGameObject->GetTransform();
        if (monsterTransform)
        {
            engine::Vector3 monsterPos = monsterTransform->GetWorldPosition();
            
            // 이펙트 프리팹 생성
            engine::GameObject* effect = engine::Prefab::Instantiate(m_effectPrefabName);
            if (effect)
            {
                // 몬스터 위치 + 인디케이터 오프셋에 배치
                if (auto* effectTransform = effect->GetTransform())
                {
                    effectTransform->SetLocalPosition(monsterPos + m_indicatorOffset);
                }
                
                // 이펙트 설정 전달
                if (auto* effectScript = effect->GetComponent<ExecutionEffectScript>())
                {
                    effectScript->SetDuration(m_effectDuration);
                    effectScript->SetScaleMultiplier(m_effectScaleMultiplier);
                }
            }
            
            // 플레이어 순간이동
            engine::Rigidbody* rigidbody = m_player->GetGameObject()->GetComponent<engine::Rigidbody>();
            if (rigidbody && rigidbody->IsDynamic())
            {
                rigidbody->ForceSetPosition(monsterPos, true);
            }
            else if (m_player->GetTransform())
            {
                m_player->GetTransform()->SetLocalPosition(monsterPos);
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
        // 몬스터 Death 타이머 시작
        // ─────────────────────────────────────────────
        m_isWaitingForDeath = true;
        m_deathTimer = 0.0f;
    }


    void ExecutionIndicatorManager::OnGui()
    {
        ImGui::InputText("Indicator Prefab", &m_indicatorPrefabName);
        ImGui::DragFloat("Raycast Distance", &m_raycastMaxDistance, 10.0f, 100.0f, 10000.0f);
        ImGui::DragFloat3("Indicator Offset", &m_indicatorOffset.x, 0.1f);

        ImGui::Separator();
        ImGui::Text("Line Settings:");
        ImGui::InputText("Line Prefab", &m_linePrefabName);
        ImGui::DragFloat("Line Player Offset", &m_linePlayerOffset, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Line Monster Offset", &m_lineMonsterOffset, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Line Base Length", &m_lineBaseLength, 0.1f, 0.01f, 10.0f);
        ImGui::DragFloat("Line Height", &m_lineHeight, 0.1f, 0.0f, 10.0f);

        ImGui::Separator();
        ImGui::Text("Execution Effect Settings:");
        ImGui::InputText("Effect Prefab", &m_effectPrefabName);
        ImGui::DragFloat("Effect Duration", &m_effectDuration, 0.05f, 0.05f, 2.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Duration of execution effect animation");
        }
        ImGui::DragFloat("Effect Scale Multiplier", &m_effectScaleMultiplier, 0.1f, 1.0f, 3.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Final scale of effect (1.5 = 150%%)");
        }
        ImGui::DragFloat("Monster Death Delay", &m_monsterDeathDelay, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Delay after teleport before monster death");
        }
        ImGui::DragInt("Trigger Wait Frames", &m_triggerWaitFramesRequired, 1, 1, 10);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Frames to wait after collider trigger change before teleport");
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

        // 처형 이펙트 설정
        j["EffectPrefabName"] = m_effectPrefabName;
        j["EffectDuration"] = m_effectDuration;
        j["EffectScaleMultiplier"] = m_effectScaleMultiplier;
        j["MonsterDeathDelay"] = m_monsterDeathDelay;
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

        // 처형 이펙트 설정
        engine::JsonGet(j, "EffectPrefabName", m_effectPrefabName);
        engine::JsonGet(j, "EffectDuration", m_effectDuration);
        engine::JsonGet(j, "EffectScaleMultiplier", m_effectScaleMultiplier);
        engine::JsonGet(j, "MonsterDeathDelay", m_monsterDeathDelay);
        engine::JsonGet(j, "TriggerWaitFramesRequired", m_triggerWaitFramesRequired);
    }
}
