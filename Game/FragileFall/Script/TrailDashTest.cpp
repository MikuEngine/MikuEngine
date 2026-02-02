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
    }

    void TrailDashTest::OnGui()
    {
        if (m_afterimage)
            ImGui::Text("Afterimage: OK (Space = dash)");
        else
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "AfterimageRenderer not found on this GameObject");
        ImGui::Text("Dashing: %s", m_isDashing ? "Yes" : "No");
    }

    void TrailDashTest::Save(engine::json& j) const
    {
        Object::Save(j);
        j["DashDuration"] = m_dashDuration;
        j["DashSpeed"] = m_dashSpeed;
    }

    void TrailDashTest::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "DashDuration", m_dashDuration, 0.2f);
        engine::JsonGet(j, "DashSpeed", m_dashSpeed, 12.0f);
    }
}