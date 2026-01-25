#include "EnginePCH.h"
#include "PathfindingAgent.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/PathfindingSystem.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
	PathfindingAgent::~PathfindingAgent()
	{
		SystemManager::Get().GetPathfindingSystem().UnregisterAgent(this);
	}

	void PathfindingAgent::Initialize()
	{
		SystemManager::Get().GetPathfindingSystem().RegisterAgent(this);
	}

	void PathfindingAgent::RequestPath(const Vector3& destination)
	{
		m_targetDestination = destination;
		m_pathRequested = true;
	}

	// 경로 상태 조회
	bool PathfindingAgent::HasPath() const
	{
		return m_hasPath;
	}
	
	bool PathfindingAgent::IsPathComplete() const
	{
		if (!m_hasPath || m_path.empty())
		{
			return false;
		}

		return m_currentPathIndex >= static_cast<int>(m_path.size()) - 1;
	}
	
	const std::vector<Vector3>& PathfindingAgent::GetPath() const
	{
		return m_path;
	}

	Vector3 PathfindingAgent::GetCurrentWaypoint() const
	{
		if (!m_hasPath || m_path.empty() || m_currentPathIndex >= static_cast<int>(m_path.size()))
		{
			return Vector3::Zero;
		}

		return m_path[m_currentPathIndex];
	}

	Vector3 PathfindingAgent::GetNextWaypoint() const
	{
		int nextIndex = m_currentPathIndex + 1;
		if (!m_hasPath || m_path.empty() || nextIndex >= static_cast<int>(m_path.size()))
		{
			return Vector3::Zero;
		}

		return m_path[nextIndex];
	}

	void PathfindingAgent::AdvanceToNextWaypoint()
	{
		if (!m_hasPath || m_path.empty())
			return;

		m_currentPathIndex++;

		if (m_currentPathIndex < static_cast<int>(m_path.size()))
		{
			m_currentWaypoint = m_path[m_currentPathIndex];
			m_hasWaypoint = true;
		}
		else
		{
			m_hasWaypoint = false;
		}
	}

	void PathfindingAgent::UpdatePathfinding(float deltaTime, const Vector3& targetPosition)
	{
		Vector3 currentPos = GetTransform()->GetWorldPosition();

		// 1. waypoint 도달 체크
		if (m_hasWaypoint)
		{
			float threshold = (m_waypointReachDistance > 0.0f) ? m_waypointReachDistance : 0.5f;
			if (IsWaypointReached(currentPos, threshold))
			{
				// waypoint 도달 - 다음 waypoint로 진행
				AdvanceToNextWaypoint();

				// 경로가 완료되었으면 재요청
				if (IsPathComplete())
				{
					RequestPath(targetPosition);
				}
			}
		}

		// 2. 주기적 경로 업데이트
		m_pathUpdateTimer += deltaTime;
		if (m_pathUpdateTimer >= m_pathUpdateInterval)
		{
			m_pathUpdateTimer = 0.0f;

			// 목표가 충분히 움직였거나 waypoint가 없을 때만 재계산
			float targetMoved = Vector3::Distance(targetPosition, m_lastTargetPos);
			if (targetMoved > m_targetMoveThreshold || !m_hasWaypoint)
			{
				RequestPath(targetPosition);
				m_lastTargetPos = targetPosition;
			}
		}
	}

	bool PathfindingAgent::GetCurrentWaypoint(Vector3& outWaypoint) const
	{
		if (!m_hasWaypoint)
		{
			return false;
		}

		outWaypoint = m_currentWaypoint;
		return true;
	}

	bool PathfindingAgent::IsWaypointReached(const Vector3& currentPos, float threshold) const
	{
		if (!m_hasWaypoint)
		{
			return false;
		}

		float usedThreshold = (threshold > 0.0f) ? threshold : m_waypointReachDistance;
		return Vector3::Distance(currentPos, m_currentWaypoint) < usedThreshold;
	}

	void PathfindingAgent::ClearPath()
	{
		m_path.clear();
		m_currentPathIndex = 0;
		m_hasPath = false;
		m_hasWaypoint = false;
	}

	// 설정
	void PathfindingAgent::SetPathUpdateInterval(float interval)
	{
		m_pathUpdateInterval = interval;
	}

	void PathfindingAgent::SetWaypointReachDistance(float distance)
	{
		m_waypointReachDistance = distance;
	}

	void PathfindingAgent::SetTargetMoveThreshold(float threshold)
	{
		m_targetMoveThreshold = threshold;
	}

	void PathfindingAgent::Update()
	{
		ProcessPathRequest();
	}

	void PathfindingAgent::OnGui()
	{
		ImGui::Text("Pathfinding Agent");
		ImGui::Separator();

		ImGui::Text("Has Path: %s", m_hasPath ? "Yes" : "No");
		ImGui::Text("Path Length: %d", static_cast<int>(m_path.size()));
		ImGui::Text("Current Index: %d", m_currentPathIndex);
		ImGui::Text("Has Waypoint: %s", m_hasWaypoint ? "Yes" : "No");

		if (m_hasWaypoint)
		{
			ImGui::Text("Current Waypoint: (%.2f, %.2f, %.2f)",
				m_currentWaypoint.x, m_currentWaypoint.y, m_currentWaypoint.z);
		}

		ImGui::Separator();
		ImGui::Text("Settings");
		ImGui::DragFloat("Update Interval", &m_pathUpdateInterval, 0.1f, 0.1f, 5.0f);
		ImGui::DragFloat("Waypoint Reach Distance", &m_waypointReachDistance, 0.1f, 0.1f, 5.0f);
		ImGui::DragFloat("Target Move Threshold", &m_targetMoveThreshold, 0.1f, 0.1f, 10.0f);
	}

	void PathfindingAgent::Save(json& j) const
	{
		Object::Save(j);

		j["PathUpdateInterval"] = m_pathUpdateInterval;
		j["WaypointReachDistance"] = m_waypointReachDistance;
		j["TargetMoveThreshold"] = m_targetMoveThreshold;
	}

	void PathfindingAgent::Load(const json& j)
	{
		Component::Load(j);
		JsonGet(j, "PathUpdateInterval", m_pathUpdateInterval);
		JsonGet(j, "WaypointReachDistance", m_waypointReachDistance);
		JsonGet(j, "TargetMoveThreshold", m_targetMoveThreshold);
	}

	void PathfindingAgent::ProcessPathRequest()
	{
		if (!m_pathRequested)
		{
			return;
		}

		m_pathRequested = false;

		Vector3 start = GetTransform()->GetWorldPosition();

		// 경로 찾기
		PathResult result = SystemManager::Get().GetPathfindingSystem().FindPath(start, m_targetDestination);

		if (result.success)
		{
			m_path = result.path;
			m_currentPathIndex = 0;
			m_hasPath = true;

			// 첫 waypoint 설정
			if (!m_path.empty())
			{
				m_currentWaypoint = m_path[0];
				m_hasWaypoint = true;
			}
		}
		else
		{
			m_hasPath = false;
			m_path.clear();
		}
	}
}