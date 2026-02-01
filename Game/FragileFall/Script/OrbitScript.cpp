#include "GamePCH.h"
#include "OrbitScript.h"

#include "AimPointerMeshScript.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void OrbitScript::Start()
    {
        CacheReferences();
        UpdateOrbit();
    }

    void OrbitScript::Update()
    {
        if (!m_playerObject || !m_aimPointerMeshObject)
        {
            CacheReferences();
            if (!m_playerObject || !m_aimPointerMeshObject)
                return;
        }

        UpdateOrbit();
    }

    void OrbitScript::CacheReferences()
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

    void OrbitScript::UpdateOrbit()
    {
        if (!m_playerObject || !m_aimPointerMeshObject)
            return;

        // ─────────────────────────────────────────────
        // 1. 피벗(회전 중심) 계산: 플레이어 XZ + 고정 Y
        // ─────────────────────────────────────────────
        engine::Vector3 playerPos = m_playerObject->GetTransform()->GetWorldPosition();
        engine::Vector3 pivot(playerPos.x, m_fixedY, playerPos.z);

        // ─────────────────────────────────────────────
        // 2. AimPointerMesh 위치 획득
        // ─────────────────────────────────────────────
        engine::Vector3 aimMeshPos = m_aimPointerMeshObject->GetTransform()->GetWorldPosition();

        // ─────────────────────────────────────────────
        // 3. 방향 벡터 계산 (XZ 평면)
        // ─────────────────────────────────────────────
        engine::Vector3 direction = aimMeshPos - pivot;
        direction.y = 0.0f;

        float length = direction.Length();
        if (length < 0.0001f)
        {
            direction = engine::Vector3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            direction.Normalize();
        }

        // ─────────────────────────────────────────────
        // 4. 위치 계산: 피벗 + 방향 × 공전반경
        // ─────────────────────────────────────────────
        engine::Vector3 position = pivot + direction * m_orbitRadius;
        GetTransform()->SetLocalPosition(position);

        // ─────────────────────────────────────────────
        // 5. 회전 계산: +Z가 AimPointerMesh를 향하도록
        // ─────────────────────────────────────────────
        float yawAngle = std::atan2(direction.x, direction.z);
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(yawAngle, 0.0f, 0.0f);
        GetTransform()->SetLocalRotation(rotation);
    }

    void OrbitScript::OnGui()
    {
        ImGui::Text("=== Orbit Script ===");
        ImGui::Separator();

        ImGui::Text("References:");
        ImGui::InputText("Player Object", &m_playerObjectName);
        ImGui::Text("Player: %s", m_playerObject ? "[OK]" : "[NOT FOUND]");

        ImGui::InputText("AimPointerMesh Object", &m_aimPointerMeshObjectName);
        ImGui::Text("AimPointerMesh: %s", m_aimPointerMeshObject ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        ImGui::Text("Settings:");
        ImGui::DragFloat("Fixed Y", &m_fixedY, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Orbit Radius", &m_orbitRadius, 0.01f, 0.0f, 10.0f);

        ImGui::Separator();

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

    void OrbitScript::Save(engine::json& j) const
    {
        Object::Save(j);

        j["PlayerObjectName"] = m_playerObjectName;
        j["AimPointerMeshObjectName"] = m_aimPointerMeshObjectName;
        j["FixedY"] = m_fixedY;
        j["OrbitRadius"] = m_orbitRadius;
    }

    void OrbitScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "PlayerObjectName", m_playerObjectName);
        engine::JsonGet(j, "AimPointerMeshObjectName", m_aimPointerMeshObjectName);
        engine::JsonGet(j, "FixedY", m_fixedY);
        engine::JsonGet(j, "OrbitRadius", m_orbitRadius);
    }
}
