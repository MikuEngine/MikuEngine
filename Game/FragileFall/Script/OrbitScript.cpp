#include "GamePCH.h"
#include "OrbitScript.h"

#include "AimPointerMeshScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>


namespace game
{
    void OrbitScript::Start()
    {
        CacheReferences();
        
        // PCS에 발사 콜백 등록
        if (m_playerController)
        {
            m_playerController->RegisterFireCallback([this]() { OnFired(); });
            
            // 발사 간격의 절반을 반동 지속 시간으로 설정
            m_recoilDuration = m_playerController->GetFireRate() / 2.0f;
        }
        
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

        // 반동 업데이트
        float deltaTime = engine::Time::DeltaTime();
        UpdateRecoil(deltaTime);

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
        
        // PlayerControllerScript 캐싱
        if (!m_playerController && m_playerObject)
        {
            m_playerController = m_playerObject->GetComponent<PlayerControllerScript>();
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
        // 4. 반동 스케일 계산 (0 → 1 → 0)
        // ─────────────────────────────────────────────
        float recoilScale = GetRecoilScale();

        // ─────────────────────────────────────────────
        // 5. 위치 계산: 피벗 + 방향 × (공전반경 + 반동Z) + 반동Y
        // ─────────────────────────────────────────────
        float effectiveRadius = m_orbitRadius + (m_recoilOffsetZ * recoilScale);
        float effectiveY = m_fixedY + (m_recoilOffsetY * recoilScale);
        
        engine::Vector3 position = pivot + direction * effectiveRadius;
        position.y = effectiveY;
        GetTransform()->SetLocalPosition(position);

        // ─────────────────────────────────────────────
        // 6. 회전 계산: +Z가 AimPointerMesh를 향하도록 + 반동 X회전
        // ─────────────────────────────────────────────
        float yawAngle = std::atan2(direction.x, direction.z);
        float pitchAngle = -(m_recoilRotationX * recoilScale) * (3.14159265f / 180.0f);  // 도 → 라디안, 위로 들리므로 -
        
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(yawAngle, pitchAngle, 0.0f);
        GetTransform()->SetLocalRotation(rotation);
    }

    // ═══════════════════════════════════════════════════════════════
    // 반동 시스템
    // ═══════════════════════════════════════════════════════════════
    void OrbitScript::OnFired()
    {
        // 반동 시작 (리셋 방식)
        m_isRecoiling = true;
        m_recoilTimer = 0.0f;
        
        // PCS에서 최신 발사 간격 읽기
        if (m_playerController)
        {
            m_recoilDuration = m_playerController->GetFireRate() / 2.0f;
        }
    }

    void OrbitScript::UpdateRecoil(float deltaTime)
    {
        if (!m_isRecoiling)
            return;

        m_recoilTimer += deltaTime;

        // 반동 완료 (전체 사이클: 0 → 1 → 0)
        if (m_recoilTimer >= m_recoilDuration * 2.0f)
        {
            m_isRecoiling = false;
            m_recoilTimer = 0.0f;
        }
    }

    float OrbitScript::GetRecoilScale() const
    {
        if (!m_isRecoiling || m_recoilDuration <= 0.0f)
            return 0.0f;

        // 전체 사이클: 0 → 1 → 0 (삼각파)
        // 0 ~ recoilDuration: 0 → 1
        // recoilDuration ~ recoilDuration*2: 1 → 0
        float totalDuration = m_recoilDuration * 2.0f;
        float t = m_recoilTimer / totalDuration;  // 0 ~ 1

        // 삼각파: 0에서 시작, 0.5에서 1, 1에서 0
        if (t < 0.5f)
        {
            return t * 2.0f;  // 0 → 1
        }
        else
        {
            return (1.0f - t) * 2.0f;  // 1 → 0
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void OrbitScript::OnGui()
    {
        ImGui::Text("=== Orbit Script ===");
        ImGui::Separator();

        ImGui::Text("References:");
        ImGui::InputText("Player Object", &m_playerObjectName);
        ImGui::Text("Player: %s", m_playerObject ? "[OK]" : "[NOT FOUND]");
        ImGui::Text("PlayerController: %s", m_playerController ? "[OK]" : "[NOT FOUND]");

        ImGui::InputText("AimPointerMesh Object", &m_aimPointerMeshObjectName);
        ImGui::Text("AimPointerMesh: %s", m_aimPointerMeshObject ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        ImGui::Text("Orbit Settings:");
        ImGui::DragFloat("Fixed Y", &m_fixedY, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Orbit Radius", &m_orbitRadius, 0.01f, 0.0f, 10.0f);

        ImGui::Separator();

        ImGui::Text("Recoil Settings:");
        ImGui::DragFloat("Recoil Z (back)", &m_recoilOffsetZ, 0.01f, -1.0f, 0.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Negative value pushes gun backward");
        }
        ImGui::DragFloat("Recoil Y (up)", &m_recoilOffsetY, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Recoil Rotation X (deg)", &m_recoilRotationX, 1.0f, 0.0f, 90.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Muzzle rise angle in degrees");
        }

        ImGui::Separator();

        ImGui::Text("Recoil State:");
        ImGui::Text("Is Recoiling: %s", m_isRecoiling ? "Yes" : "No");
        ImGui::Text("Recoil Timer: %.3f / %.3f", m_recoilTimer, m_recoilDuration * 2.0f);
        ImGui::Text("Recoil Scale: %.3f", GetRecoilScale());
        
        if (m_playerController)
        {
            ImGui::Text("Fire Rate: %.3f sec", m_playerController->GetFireRate());
        }

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
        
        // 반동 설정
        j["RecoilOffsetZ"] = m_recoilOffsetZ;
        j["RecoilOffsetY"] = m_recoilOffsetY;
        j["RecoilRotationX"] = m_recoilRotationX;
    }

    void OrbitScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "PlayerObjectName", m_playerObjectName);
        engine::JsonGet(j, "AimPointerMeshObjectName", m_aimPointerMeshObjectName);
        engine::JsonGet(j, "FixedY", m_fixedY);
        engine::JsonGet(j, "OrbitRadius", m_orbitRadius);
        
        // 반동 설정
        engine::JsonGet(j, "RecoilOffsetZ", m_recoilOffsetZ);
        engine::JsonGet(j, "RecoilOffsetY", m_recoilOffsetY);
        engine::JsonGet(j, "RecoilRotationX", m_recoilRotationX);
    }
}
