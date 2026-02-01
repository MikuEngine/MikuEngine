#include "GamePCH.h"
#include "PlayerAimMeshController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void PlayerAimMeshController::Start()
    {
        CacheReferences();
        UpdatePositionAndRotation();
    }

    void PlayerAimMeshController::Update()
    {
        // 참조가 없으면 다시 시도
        if (!m_playerObject || !m_aimPointerMeshObject)
        {
            CacheReferences();
            if (!m_playerObject || !m_aimPointerMeshObject)
                return;
        }

        UpdatePositionAndRotation();
    }

    void PlayerAimMeshController::CacheReferences()
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene)
            return;

        if (!m_playerObject)
        {
            m_playerObject = scene->FindGameObject(m_playerObjectName);
        }

        if (!m_aimPointerMeshObject)
        {
            m_aimPointerMeshObject = scene->FindGameObject(m_aimPointerMeshObjectName);
        }
    }

    void PlayerAimMeshController::UpdatePositionAndRotation()
    {
        if (!m_playerObject || !m_aimPointerMeshObject)
            return;

        // ─────────────────────────────────────────────
        // 1. 위치: 플레이어 Transform의 XZ + 고정 Y
        // ─────────────────────────────────────────────
        engine::Vector3 playerPos = m_playerObject->GetTransform()->GetWorldPosition();
        engine::Vector3 newPosition(playerPos.x, m_fixedY, playerPos.z);
        GetTransform()->SetLocalPosition(newPosition);

        // ─────────────────────────────────────────────
        // 2. AimPointerMesh 위치 획득
        // ─────────────────────────────────────────────
        engine::Vector3 aimMeshPos = m_aimPointerMeshObject->GetTransform()->GetWorldPosition();

        // ─────────────────────────────────────────────
        // 3. 방향 벡터 계산 (XZ 평면)
        // ─────────────────────────────────────────────
        engine::Vector3 direction = aimMeshPos - newPosition;
        direction.y = 0.0f;  // Y 성분 제거

        float length = direction.Length();
        if (length < 0.0001f)
        {
            // 방향이 너무 짧으면 기본 방향 사용 (+Z)
            direction = engine::Vector3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            direction.Normalize();
        }

        // ─────────────────────────────────────────────
        // 4. 회전 계산: +Z가 AimPointerMesh를 향하도록
        // ─────────────────────────────────────────────
        // Y축 회전각 (Yaw) = atan2(x, z)
        float yawAngle = std::atan2(direction.x, direction.z);

        // Quaternion 생성 (Yaw, Pitch, Roll 순서)
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(yawAngle, 0.0f, 0.0f);
        GetTransform()->SetLocalRotation(rotation);
    }

    void PlayerAimMeshController::OnGui()
    {
        ImGui::Text("=== PlayerAimMesh Controller ===");
        ImGui::Separator();

        // 참조 설정
        ImGui::Text("References:");
        ImGui::InputText("Player Object", &m_playerObjectName);
        ImGui::Text("Player: %s", m_playerObject ? "[OK]" : "[NOT FOUND]");

        ImGui::InputText("AimPointerMesh Object", &m_aimPointerMeshObjectName);
        ImGui::Text("AimPointerMesh: %s", m_aimPointerMeshObject ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        // 설정
        ImGui::Text("Settings:");
        ImGui::DragFloat("Fixed Y", &m_fixedY, 0.1f, -100.0f, 100.0f);

        ImGui::Separator();

        // 현재 상태 표시
        ImGui::Text("Current State:");
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        engine::Vector3 forward = GetTransform()->GetForward();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Forward:  (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);

        if (m_playerObject)
        {
            engine::Vector3 playerPos = m_playerObject->GetTransform()->GetWorldPosition();
            ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", playerPos.x, playerPos.y, playerPos.z);
        }

        if (m_aimPointerMeshObject)
        {
            engine::Vector3 aimMeshPos = m_aimPointerMeshObject->GetTransform()->GetWorldPosition();
            ImGui::Text("AimMesh Pos: (%.2f, %.2f, %.2f)", aimMeshPos.x, aimMeshPos.y, aimMeshPos.z);
        }
    }

    void PlayerAimMeshController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["PlayerObjectName"] = m_playerObjectName;
        j["AimPointerMeshObjectName"] = m_aimPointerMeshObjectName;
        j["FixedY"] = m_fixedY;
    }

    void PlayerAimMeshController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "PlayerObjectName", m_playerObjectName);
        engine::JsonGet(j, "AimPointerMeshObjectName", m_aimPointerMeshObjectName);
        engine::JsonGet(j, "FixedY", m_fixedY);
    }
}
