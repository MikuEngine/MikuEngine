#include "GamePCH.h"
#include "PathfindTest.h"

#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>

namespace game
{
    void PathfindTest::Start()
    {
        m_agent = GetGameObject()->GetComponent<engine::PathfindingAgent>();
        m_target = engine::GameObject::Find("Player");
    }
    void PathfindTest::Update()
    {
        if (!m_agent || !m_target)
            return;

        engine::Vector3 targetPos = m_target->GetTransform()->GetWorldPosition();

        // 경로 업데이트 (1초마다)
        m_agent->UpdatePathfinding(engine::Time::DeltaTime(), targetPos);

        // Waypoint로 이동
        engine::Vector3 waypoint;
        if (m_agent->GetCurrentWaypoint(waypoint))
        {
            engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
            engine::Vector3 direction = (waypoint - currentPos);
            float distance = direction.Length();

            if (distance > 0.01f)
            {
                direction.Normalize();
                engine::Vector3 movement = direction * m_moveSpeed * engine::Time::DeltaTime();
                GetTransform()->Translate(movement, false);
            }
        }
    }
    void PathfindTest::OnGui()
    {
    }

    void PathfindTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void PathfindTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}