#include "GamePCH.h"
#include "ExecutionIndicatorManager.h"

#include "Script/CharacterScript/Monster/MonsterScript.h"

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
    }

    void ExecutionIndicatorManager::Update()
    {
        if (!m_mainCamera) return;

        // 처형 애니메이션 중이면 애니메이션만 처리
        if (m_isExecuting)
        {
            UpdateExecution(engine::Time::DeltaTime());
            return;
        }

        // 마우스 아래의 Fragile 몬스터 확인
        MonsterScript* fragileMonster = GetFragileMonsterUnderMouse();

        if (fragileMonster)
        {
            // Fragile 몬스터 위에 마우스가 있음
            if (m_hoveredMonster.Get() != fragileMonster)
            {
                // 새로운 몬스터로 변경
                m_hoveredMonster = fragileMonster;
                
                // 인디케이터 생성/표시
                if (!m_indicatorInstance)
                {
                    CreateIndicatorInstance();
                }
            }

            // 인디케이터 위치 업데이트
            if (m_indicatorInstance && fragileMonster->GetTransform())
            {
                ShowIndicator(fragileMonster->GetTransform());
            }

            // 우클릭 시 처형 시작
            if (engine::Input::IsMousePressed(engine::Input::Buttons::RIGHT))
            {
                StartExecution(fragileMonster);
            }
        }
        else
        {
            // Fragile 몬스터 위에 마우스가 없음
            if (m_hoveredMonster)
            {
                m_hoveredMonster = nullptr;
                HideIndicator();
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
            LOG_PRINT("[ExecutionIndicatorManager] ERROR: Failed to instantiate prefab '%s'", m_indicatorPrefabName.c_str());
            return;
        }

        m_indicatorInstance = indicator;
        m_indicatorTransform = indicator->GetTransform();

        // 초기에는 숨김
        indicator->SetActive(false);
    }

    void ExecutionIndicatorManager::DestroyIndicatorInstance()
    {
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

    MonsterScript* ExecutionIndicatorManager::GetFragileMonsterUnderMouse()
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
                        // Fragile 상태인지 확인
                        if (monster->m_isFragile && !monster->m_isDead)
                        {
                            return monster;
                        }
                        // MonsterScript를 찾았지만 Fragile이 아니면 다음 히트로
                        break;
                    }
                }
                transform = transform->GetParent();
            }
        }

        return nullptr;
    }

    void ExecutionIndicatorManager::StartExecution(MonsterScript* monster)
    {
        if (!monster || m_isExecuting) return;

        m_isExecuting = true;
        m_executionTimer = 0.0f;
        m_executingMonster = monster;

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
        if (!m_executingMonster)
        {
            // 몬스터가 사라짐, 처형 취소
            m_isExecuting = false;
            HideIndicator();
        }
    }

    void ExecutionIndicatorManager::FinishExecution()
    {
        m_isExecuting = false;

        // 인디케이터 숨김
        HideIndicator();

        // 몬스터 Dead 상태로 전이
        if (m_executingMonster)
        {
            m_executingMonster->TriggerDeath();
        }

        // 상태 초기화
        m_executingMonster = nullptr;
        m_hoveredMonster = nullptr;
    }

    void ExecutionIndicatorManager::OnGui()
    {
        ImGui::InputText("Indicator Prefab", &m_indicatorPrefabName);
        ImGui::DragFloat("Rotation Duration", &m_rotationDuration, 0.1f, 0.1f, 5.0f);
        ImGui::DragFloat("Raycast Distance", &m_raycastMaxDistance, 10.0f, 100.0f, 10000.0f);
        ImGui::DragFloat3("Indicator Offset", &m_indicatorOffset.x, 0.1f);

        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Hovered Monster: %s", m_hoveredMonster ? "Yes" : "No");
        ImGui::Text("Is Executing: %s", m_isExecuting ? "Yes" : "No");
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
    }

    void ExecutionIndicatorManager::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "IndicatorPrefabName", m_indicatorPrefabName);
        engine::JsonGet(j, "RotationDuration", m_rotationDuration);
        engine::JsonGet(j, "RaycastMaxDistance", m_raycastMaxDistance);
        engine::JsonGet(j, "IndicatorOffset", m_indicatorOffset);
    }
}
