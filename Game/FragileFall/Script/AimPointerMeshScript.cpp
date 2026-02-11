#include "GamePCH.h"
#include "AimPointerMeshScript.h"

#include "AimModeController.h"
#include "Script/Interface/IDamageable.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossBigProjectile.h"

#include <Framework/Object/Component/Camera.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <algorithm>

namespace game
{
    void AimPointerMeshScript::Start()
    {
        CacheAimPointer();
        CacheCameraAndCollider();

        if (m_aimPointer)
        {
            // 같은 마우스 레이를 Y=0 평면까지 연장한 교점을 사용한다.
            engine::Vector3 meshPos = m_aimPointer->GetWorldPosition();
            if (!m_aimPointer->TryGetMouseRayPlaneIntersection(0.0f, meshPos))
            {
                // 실패 시에만 폴백
                meshPos.y = 0.0f;
            }
            GetTransform()->SetLocalPosition(meshPos);
        }

        // 기준 거리는 시작 시점의 카메라-포인터 거리로 잡는다.
        // (이전 하단 중앙 레이 기준은 기준 거리가 과도하게 작아져 스케일 폭증을 만들 수 있음)
        if (m_mainCamera)
        {
            const engine::Vector3 camPos = m_mainCamera->GetTransform()->GetWorldPosition();
            const engine::Vector3 meshPos = GetTransform()->GetWorldPosition();
            m_scaleRefDistance = engine::Vector3::Distance(camPos, meshPos);
            if (m_scaleRefDistance < 0.0001f)
                m_scaleRefDistance = 1.0f;
        }

        if (m_collider)
        {
            m_collider->SetSyncWithTransform(true);
            m_collider->CheckAndSyncTransformScale();
        }

        UpdateDistanceBasedScale();
    }

    void AimPointerMeshScript::Update()
    {
        // AimPointer 참조가 없으면 다시 시도
        if (!m_aimPointer)
        {
            CacheAimPointer();
            if (!m_aimPointer)
                return;
        }

        if (!m_mainCamera || !m_collider)
            CacheCameraAndCollider();

        // 같은 마우스 레이를 Y=0 평면까지 연장한 교점을 사용한다.
        engine::Vector3 meshPos = m_aimPointer->GetWorldPosition();
        if (!m_aimPointer->TryGetMouseRayPlaneIntersection(0.0f, meshPos))
        {
            meshPos.y = 0.0f;
        }
        GetTransform()->SetLocalPosition(meshPos);
        UpdateDistanceBasedScale();

        RefreshOnEnemyState();
    }

    void AimPointerMeshScript::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (!info.gameObject)
            return;

        if (!IsValidOnEnemyTarget(info.gameObject))
            return;

        const auto it = std::find_if(
            m_overlapTargets.begin(),
            m_overlapTargets.end(),
            [&](const engine::Ptr<engine::GameObject>& target)
            {
                return target == info.gameObject;
            }
        );

        if (it == m_overlapTargets.end())
        {
            m_overlapTargets.emplace_back(info.gameObject);
        }

        RefreshOnEnemyState();
    }

    void AimPointerMeshScript::OnTriggerExit(const engine::CollisionInfo& info)
    {
        if (!info.gameObject)
            return;

        m_overlapTargets.erase(
            std::remove_if(
                m_overlapTargets.begin(),
                m_overlapTargets.end(),
                [&](const engine::Ptr<engine::GameObject>& target)
                {
                    return !target || target == info.gameObject;
                }
            ),
            m_overlapTargets.end()
        );

        RefreshOnEnemyState();
    }

    engine::Vector3 AimPointerMeshScript::GetWorldPosition() const
    {
        return GetTransform()->GetWorldPosition();
    }

    void AimPointerMeshScript::CacheAimPointer()
    {
        if (m_aimPointer)
            return;

        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene)
            return;

        if (auto* aimGO = scene->FindGameObject(m_aimPointerObjectName))
        {
            m_aimPointer = aimGO->GetComponent<AimModeController>();
        }
    }

    void AimPointerMeshScript::CacheCameraAndCollider()
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene)
            return;

        if (!m_mainCamera)
        {
            if (auto* camGO = scene->FindGameObject("MainCamera"))
            {
                m_mainCamera = camGO->GetComponent<engine::Camera>();
            }
        }

        if (!m_collider)
        {
            m_collider = GetGameObject()->GetComponent<engine::Collider>();
        }
    }

    void AimPointerMeshScript::UpdateDistanceBasedScale()
    {
        if (!m_mainCamera)
            return;

        if (m_scaleRefDistance < 0.0001f)
            m_scaleRefDistance = 1.0f;

        const engine::Vector3 camPos = m_mainCamera->GetTransform()->GetWorldPosition();
        const engine::Vector3 meshPos = GetTransform()->GetWorldPosition();
        const float currentDistance = engine::Vector3::Distance(camPos, meshPos);

        float scale = m_scaleAtNearest * (currentDistance / m_scaleRefDistance);
        scale = std::clamp(scale, m_scaleMin, m_scaleMax);

        GetTransform()->SetLocalScale(engine::Vector3(scale, scale, scale));
        if (m_collider)
            m_collider->CheckAndSyncTransformScale();
    }

    bool AimPointerMeshScript::IsValidOnEnemyTarget(const engine::Ptr<engine::GameObject>& other) const
    {
        if (!other)
            return false;

        // 플레이어는 항상 제외
        if (other->GetComponent<PlayerControllerScript>())
            return false;

        auto* damageable = other->GetInterface<IDamageable>();
        if (!damageable)
            return false;

        // Fragile 계열은 OnEnemy 제외
        if (auto* monster = dynamic_cast<MonsterScript*>(damageable))
        {
            return !monster->m_isFragile && !monster->m_isDead;
        }
        if (auto* pillar = dynamic_cast<BossPillar*>(damageable))
        {
            return !pillar->IsCrystalized();
        }
        if (auto* bigProjectile = dynamic_cast<BossBigProjectile*>(damageable))
        {
            return !bigProjectile->IsCrystallized() && !bigProjectile->IsDestroyed();
        }
        // 그 외 IDamageable은 기본적으로 유효 대상으로 취급
        return true;
    }

    void AimPointerMeshScript::RefreshOnEnemyState()
    {
        m_overlapTargets.erase(
            std::remove_if(
                m_overlapTargets.begin(),
                m_overlapTargets.end(),
                [&](const engine::Ptr<engine::GameObject>& target)
                {
                    return !target || !IsValidOnEnemyTarget(target);
                }
            ),
            m_overlapTargets.end()
        );

        const bool isOnEnemy = !m_overlapTargets.empty();
        m_lastOnEnemy = isOnEnemy;
    }

    void AimPointerMeshScript::OnGui()
    {
        ImGui::Text("=== AimPointerMesh Script ===");
        ImGui::Separator();

        // 참조 설정
        ImGui::Text("References:");
        ImGui::InputText("AimPointer Object", &m_aimPointerObjectName);
        ImGui::Text("AimPointer: %s", m_aimPointer ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        // 하위호환용 파라미터 표시 (현재 동작에서는 미사용)
        ImGui::Text("Settings:");
        ImGui::BeginDisabled(true);
        ImGui::DragFloat("Fixed Y (Legacy, Unused)", &m_fixedY, 0.1f, -100.0f, 100.0f);
        ImGui::EndDisabled();
        ImGui::Text("Position Source: MouseRay @ PlaneY=0 intersection");
        ImGui::DragFloat("Scale @ Nearest", &m_scaleAtNearest, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Scale Min", &m_scaleMin, 0.01f, 0.001f, 10.0f);
        ImGui::DragFloat("Scale Max", &m_scaleMax, 0.1f, 0.1f, 100.0f);
        if (m_scaleMin > m_scaleMax)
            m_scaleMax = m_scaleMin;
        ImGui::Text("Scale Ref Distance: %.3f", m_scaleRefDistance);

        ImGui::Separator();

        // 현재 상태 표시
        ImGui::Text("Current State:");
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

        if (m_aimPointer)
        {
            engine::Vector3 aimPos = m_aimPointer->GetWorldPosition();
            ImGui::Text("AimPointer Pos: (%.2f, %.2f, %.2f)", aimPos.x, aimPos.y, aimPos.z);
        }
    }

    void AimPointerMeshScript::Save(engine::json& j) const
    {
        Object::Save(j);

        j["AimPointerObjectName"] = m_aimPointerObjectName;
        j["FixedY"] = m_fixedY;
        j["ScaleAtNearest"] = m_scaleAtNearest;
        j["ScaleMin"] = m_scaleMin;
        j["ScaleMax"] = m_scaleMax;
    }

    void AimPointerMeshScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "AimPointerObjectName", m_aimPointerObjectName);
        engine::JsonGet(j, "FixedY", m_fixedY);
        engine::JsonGet(j, "ScaleAtNearest", m_scaleAtNearest);
        engine::JsonGet(j, "ScaleMin", m_scaleMin);
        engine::JsonGet(j, "ScaleMax", m_scaleMax);
    }
}
