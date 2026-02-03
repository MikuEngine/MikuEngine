#include "GamePCH.h"
#include "TrailDashTest.h"

#include <Engine/Framework/Object/Component/Renderer/AfterimageRenderer.h>
#include <Engine/Framework/Object/GameObject/GameObject.h>
#include <Engine/Framework/Object/Component/Transform.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    void TrailDashTest::Start()
    {
        m_afterimage = GetGameObject()->GetComponent<engine::AfterimageRenderer>();
    }

    void TrailDashTest::Update()
    {
        if (!GetTransform())
            return;

        if (m_isDashing)
        {
            const float dt = engine::Time::DeltaTime();
            m_dashTimer += dt;

            engine::Vector3 pos = GetTransform()->GetWorldPosition();
            pos += m_dashDirection * (m_dashSpeed * dt);
            engine::Matrix world = GetTransform()->GetWorld();
            world._41 = pos.x;
            world._42 = pos.y;
            world._43 = pos.z;
            GetTransform()->SetWorldMatrix(world);

            if (m_afterimage)
                m_afterimage->RecordSample();

            if (m_dashTimer >= m_dashDuration)
            {
                m_isDashing = false;
                if (m_afterimage)
                    m_afterimage->EndRecording();
            }
            return;
        }

        if (engine::Input::IsKeyPressed(engine::Keys::Space))
        {
            m_dashDirection = GetTransform()->GetForward();
            m_dashDirection.Normalize();
            m_dashTimer = 0.0f;
            m_isDashing = true;
            if (m_afterimage)
                m_afterimage->BeginRecording();
        }

        // T = 순간이동 (이전 위치→현재 방향 앞쪽으로 이동, 구간에 잔상만 남김)
        if (engine::Input::IsKeyPressed(engine::Keys::T) && m_afterimage)
        {
            engine::Matrix fromWorld = GetTransform()->GetWorld();
            engine::Vector3 forward = GetTransform()->GetForward();
            forward.Normalize();
            engine::Vector3 toPos = fromWorld.Translation() + forward * m_teleportDistance;
            engine::Matrix toWorld = fromWorld;
            toWorld._41 = toPos.x;
            toWorld._42 = toPos.y;
            toWorld._43 = toPos.z;

            m_afterimage->ClearSlices();
            m_afterimage->RecordTeleportPath(fromWorld, toWorld, m_teleportNumSlices);
            GetTransform()->SetWorldMatrix(toWorld);
        }
    }

    void TrailDashTest::OnGui()
    {
        if (m_afterimage)
            ImGui::Text("Afterimage: OK (Space = dash, T = teleport)");
        else
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "AfterimageRenderer not found on this GameObject");
        ImGui::Text("Dashing: %s", m_isDashing ? "Yes" : "No");

        ImGui::DragFloat("Dash Duration", &m_dashDuration, 0.001f, 0.001f, 100.0f);
        ImGui::DragFloat("Dash Speed", &m_dashSpeed, 0.1f, 0.1f, 100.0f);
        ImGui::Separator();
        ImGui::TextUnformatted("Teleport (T key)");
        ImGui::DragFloat("Teleport Distance", &m_teleportDistance, 0.5f, 0.5f, 50.0f);
        int numSlices = static_cast<int>(m_teleportNumSlices);
        if (ImGui::SliderInt("Teleport Trail Slices", &numSlices, 1, 32))
            m_teleportNumSlices = static_cast<size_t>(numSlices);
    }

    void TrailDashTest::Save(engine::json& j) const
    {
        Object::Save(j);
        j["DashDuration"] = m_dashDuration;
        j["DashSpeed"] = m_dashSpeed;
        j["TeleportDistance"] = m_teleportDistance;
        j["TeleportNumSlices"] = static_cast<int>(m_teleportNumSlices);
    }

    void TrailDashTest::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "DashDuration", m_dashDuration, 0.2f);
        engine::JsonGet(j, "DashSpeed", m_dashSpeed, 12.0f);
        engine::JsonGet(j, "TeleportDistance", m_teleportDistance, 5.0f);
        int n = 12;
        engine::JsonGet(j, "TeleportNumSlices", n, 12);
        m_teleportNumSlices = static_cast<size_t>(std::max(1, n));
    }
}