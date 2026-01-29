#include "GamePCH.h"
#include "ExecutionIndicatorManager.h"

#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossProjectile.h"

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
    }

    void ExecutionIndicatorManager::Update()
    {
        if (!m_mainCamera) return;

        // 대시 중이면 대시 처리
        if (m_isDashing)
        {
            UpdateDash(engine::Time::DeltaTime());
            return;
        }

        // 처형 애니메이션 중이면 애니메이션만 처리
        if (m_isExecuting)
        {
            UpdateExecution(engine::Time::DeltaTime());
            return;
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

            // 거리에 따른 표시 처리
            bool isInRange = IsMonsterInExecutionRange(fragileMonster);
            
            // 라인은 항상 업데이트 및 표시
            if (m_player && m_lineInstance && fragileMonster->GetTransform())
            {
                engine::Vector3 monsterPos = fragileMonster->GetTransform()->GetWorldPosition();
                UpdateLine(monsterPos);
                ShowLine();
            }
            
            // 인디케이터는 처형 사거리 내에서만 표시
            if (isInRange)
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
        if (!target || m_isExecuting || m_isDashing || !m_player) return;

        m_executingGameObject = target;

        // ─────────────────────────────────────────────
        // 플레이어 Execution 스테이트로 전이 (순간이동은 대시에서 처리)
        // ─────────────────────────────────────────────
        engine::LogicFSM* playerFSM = m_player->GetGameObject()->GetComponent<engine::LogicFSM>();
        if (playerFSM)
        {
            playerFSM->SetTrigger("ExecuteMonster");
        }

        // ─────────────────────────────────────────────
        // 대시 순간이동 시작
        // ─────────────────────────────────────────────
        m_isDashing = true;
        m_dashTimer = 0.0f;
        
        // 첫 번째 대시 즉시 실행
        PerformDash();
    }

    // ═══════════════════════════════════════════════════════════════
    // 대시 순간이동
    // ═══════════════════════════════════════════════════════════════

    void ExecutionIndicatorManager::UpdateDash(float deltaTime)
    {
        if (!m_isDashing || !m_executingGameObject) return;

        m_dashTimer += deltaTime;

        // 대시 간격마다 순간이동
        if (m_dashTimer >= m_dashInterval)
        {
            m_dashTimer = 0.0f;
            PerformDash();
        }
    }

    void ExecutionIndicatorManager::PerformDash()
    {
        if (!m_executingGameObject || !m_player) return;

        engine::Transform* playerTransform = m_player->GetTransform();
        engine::Transform* monsterTransform = m_executingGameObject->GetTransform();
        
        if (!playerTransform || !monsterTransform) return;

        engine::Vector3 playerPos = playerTransform->GetWorldPosition();
        engine::Vector3 monsterPos = monsterTransform->GetWorldPosition();
        
        engine::Vector3 direction = monsterPos - playerPos;
        float distance = direction.Length();
        
        // 남은 거리가 최종 도달 거리 이하면 최종 순간이동
        if (distance <= m_finalDashThreshold)
        {
            FinishDash();
            return;
        }
        
        // 몬스터 방향으로 대시 거리만큼 순간이동
        direction.Normalize();
        engine::Vector3 targetPos = playerPos + direction * m_dashDistance;
        
        // Rigidbody를 통해 순간이동
        engine::Rigidbody* rigidbody = m_player->GetGameObject()->GetComponent<engine::Rigidbody>();
        if (rigidbody && rigidbody->IsDynamic())
        {
            rigidbody->ForceSetPosition(targetPos, true);
        }
        else
        {
            playerTransform->SetLocalPosition(targetPos);
        }
    }

    void ExecutionIndicatorManager::FinishDash()
    {
        if (!m_executingGameObject || !m_player) return;

        // ─────────────────────────────────────────────
        // 몬스터의 모든 콜라이더를 트리거로 변경 (최종 순간이동 직전)
        // ─────────────────────────────────────────────
        engine::GameObject* monsterGO = m_executingGameObject.Get();
        if (monsterGO)
        {
            if (auto* boxCollider = monsterGO->GetComponent<engine::BoxCollider>())
            {
                boxCollider->SetIsTrigger(true);
            }
            if (auto* sphereCollider = monsterGO->GetComponent<engine::SphereCollider>())
            {
                sphereCollider->SetIsTrigger(true);
            }
            if (auto* capsuleCollider = monsterGO->GetComponent<engine::CapsuleCollider>())
            {
                capsuleCollider->SetIsTrigger(true);
            }

            if (auto comp = m_executingGameObject->GetComponent<BossProjectile>())
            {
                comp->Execute();
            }
        }

        // ─────────────────────────────────────────────
        // 몬스터 위치로 최종 순간이동
        // ─────────────────────────────────────────────
        engine::Transform* monsterTransform = m_executingGameObject->GetTransform();
        if (monsterTransform)
        {
            engine::Vector3 monsterPos = monsterTransform->GetWorldPosition();
            
            engine::Rigidbody* rigidbody = m_player->GetGameObject()->GetComponent<engine::Rigidbody>();
            if (rigidbody && rigidbody->IsDynamic())
            {
                rigidbody->ForceSetPosition(monsterPos, true);
            }
            else if (m_player->GetTransform())
            {
                m_player->GetTransform()->SetLocalPosition(monsterPos);
            }
        }

        // ─────────────────────────────────────────────
        // 대시 종료, 처형 애니메이션 시작
        // ─────────────────────────────────────────────
        m_isDashing = false;
        m_dashTimer = 0.0f;
        
        m_isExecuting = true;
        m_executionTimer = 0.0f;
        
        // 현재 인디케이터 회전 저장
        if (m_indicatorTransform)
        {
            m_initialRotation = m_indicatorTransform->GetLocalRotation();
        }
    }

    void ExecutionIndicatorManager::UpdateExecution(float deltaTime)
    {
        if (!m_isExecuting) return;

        m_executionTimer += deltaTime;

        // 진행률 계산 (0.0 ~ 1.0)
        float progress = m_executionTimer / m_rotationDuration;

        if (progress >= 1.0f)
        {
            // 애니메이션 완료
            FinishExecution();
            return;
        }

        // Y축 회전 애니메이션 (360도)
        if (m_indicatorTransform)
        {
            float rotationAngle = progress * DirectX::XM_2PI;  // 0 ~ 2π (360도)
            
            // 초기 회전에 Y축 회전 추가
            engine::Quaternion yRotation = engine::Quaternion::CreateFromAxisAngle(
                engine::Vector3::UnitY, rotationAngle);
            
            m_indicatorTransform->SetLocalRotation(m_initialRotation * yRotation);
        }

        // 몬스터가 파괴되었는지 확인
        if (!m_executingGameObject)
        {
            // 몬스터가 사라짐, 처형 취소
            m_isExecuting = false;
            HideIndicator();
        }
    }

    void ExecutionIndicatorManager::FinishExecution()
    {
        m_isExecuting = false;

        // 인디케이터와 라인 숨김
        HideIndicator();
        HideLine();

        // 몬스터 Dead 상태로 전이
        if (m_executingGameObject)
        {
            if (auto comp = m_executingGameObject->GetComponent<MonsterScript>())
            {
                comp->TriggerDeath();
            }
            else if (auto comp = m_executingGameObject->GetComponent<BossPillar>())
            {
                comp->Execute();
            }
        }

        // 플레이어 Execution 상태 종료
        if (m_player)
        {
            engine::LogicFSM* playerFSM = m_player->GetGameObject()->GetComponent<engine::LogicFSM>();
            if (playerFSM)
            {
                playerFSM->SetTrigger("ExecutionComplete");
            }
        }

        // 상태 초기화
        m_executingGameObject = nullptr;
        m_hoveredGameObject = nullptr;
    }

    void ExecutionIndicatorManager::OnGui()
    {
        ImGui::InputText("Indicator Prefab", &m_indicatorPrefabName);
        ImGui::DragFloat("Rotation Duration", &m_rotationDuration, 0.1f, 0.1f, 5.0f);
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
        ImGui::Text("Dash Settings:");
        ImGui::DragFloat("Dash Distance", &m_dashDistance, 0.1f, 0.5f, 10.0f);
        ImGui::DragFloat("Dash Interval", &m_dashInterval, 0.01f, 0.05f, 1.0f);
        ImGui::DragFloat("Final Dash Threshold", &m_finalDashThreshold, 0.1f, 0.5f, 10.0f);

        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Hovered Monster: %s", m_hoveredGameObject ? "Yes" : "No");
        ImGui::Text("Is Dashing: %s", m_isDashing ? "Yes" : "No");
        ImGui::Text("Is Executing: %s", m_isExecuting ? "Yes" : "No");
        ImGui::Text("Player Found: %s", m_player ? "Yes" : "No");
        ImGui::Text("Line Instance: %s", m_lineInstance ? "Yes" : "No");
        if (m_isDashing)
        {
            ImGui::Text("Dash Timer: %.2f / %.2f", m_dashTimer, m_dashInterval);
        }
        if (m_isExecuting)
        {
            ImGui::Text("Execution Progress: %.1f%%", (m_executionTimer / m_rotationDuration) * 100.0f);
        }
    }

    void ExecutionIndicatorManager::Save(engine::json& j) const
    {
        Object::Save(j);
        j["IndicatorPrefabName"] = m_indicatorPrefabName;
        j["RotationDuration"] = m_rotationDuration;
        j["RaycastMaxDistance"] = m_raycastMaxDistance;
        j["IndicatorOffset"] = m_indicatorOffset;

        // 라인 설정
        j["LinePrefabName"] = m_linePrefabName;
        j["LinePlayerOffset"] = m_linePlayerOffset;
        j["LineMonsterOffset"] = m_lineMonsterOffset;
        j["LineBaseLength"] = m_lineBaseLength;
        j["LineHeight"] = m_lineHeight;

        // 대시 설정
        j["DashDistance"] = m_dashDistance;
        j["DashInterval"] = m_dashInterval;
        j["FinalDashThreshold"] = m_finalDashThreshold;
    }

    void ExecutionIndicatorManager::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "IndicatorPrefabName", m_indicatorPrefabName);
        engine::JsonGet(j, "RotationDuration", m_rotationDuration);
        engine::JsonGet(j, "RaycastMaxDistance", m_raycastMaxDistance);
        engine::JsonGet(j, "IndicatorOffset", m_indicatorOffset);

        // 라인 설정
        engine::JsonGet(j, "LinePrefabName", m_linePrefabName);
        engine::JsonGet(j, "LinePlayerOffset", m_linePlayerOffset);
        engine::JsonGet(j, "LineMonsterOffset", m_lineMonsterOffset);
        engine::JsonGet(j, "LineBaseLength", m_lineBaseLength);
        engine::JsonGet(j, "LineHeight", m_lineHeight);

        // 대시 설정
        engine::JsonGet(j, "DashDistance", m_dashDistance);
        engine::JsonGet(j, "DashInterval", m_dashInterval);
        engine::JsonGet(j, "FinalDashThreshold", m_finalDashThreshold);
    }
}
