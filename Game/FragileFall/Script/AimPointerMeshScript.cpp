#include "GamePCH.h"
#include "AimPointerMeshScript.h"

#include "AimModeController.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void AimPointerMeshScript::Start()
    {
        CacheAimPointer();

        if (m_aimPointer)
        {
            engine::Vector3 aimWorldPos = m_aimPointer->GetWorldPosition();
            GetTransform()->SetLocalPosition(engine::Vector3(aimWorldPos.x, m_fixedY, aimWorldPos.z));
        }
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

        // AimPointer의 월드 좌표 획득
        engine::Vector3 aimWorldPos = m_aimPointer->GetWorldPosition();

        // Y는 고정값, XZ만 AimPointer 좌표 사용
        engine::Vector3 newPosition(aimWorldPos.x, m_fixedY, aimWorldPos.z);

        // Transform 직접 설정 (순간이동)
        GetTransform()->SetLocalPosition(newPosition);
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

    void AimPointerMeshScript::OnGui()
    {
        ImGui::Text("=== AimPointerMesh Script ===");
        ImGui::Separator();

        // 참조 설정
        ImGui::Text("References:");
        ImGui::InputText("AimPointer Object", &m_aimPointerObjectName);
        ImGui::Text("AimPointer: %s", m_aimPointer ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        // Y 고정값 설정
        ImGui::Text("Settings:");
        ImGui::DragFloat("Fixed Y", &m_fixedY, 0.1f, -100.0f, 100.0f);

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
    }

    void AimPointerMeshScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "AimPointerObjectName", m_aimPointerObjectName);
        engine::JsonGet(j, "FixedY", m_fixedY);
    }
}
