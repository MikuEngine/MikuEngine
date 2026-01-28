#include "EnginePCH.h"
#include "PathfindingSystem.h"

#include <queue>
#include <unordered_set>
#include <utility>

#include "Framework/Object/Component/Pathfinding/PathfindingAgent.h"

namespace engine
{
	PathResult PathfindingSystem::FindPath(const Vector3& start, const Vector3& end)
	{
        PathResult result;

        if (m_components.empty())
        {
            result.success = false;
            return result;
        }

        const GridMap* gridMap = m_components[0];

        if (!gridMap || !gridMap->IsActive())
        {
            result.success = false;
            return result;
        }

        float directDistance = Vector3::Distance(start, end);

        // 월드 좌표를 그리드 좌표로 변환
        int startX, startZ, endX, endZ;
        gridMap->WorldToGrid(start, startX, startZ);
        gridMap->WorldToGrid(end, endX, endZ);

        bool usedSnap = false;
        bool usedEndSnap = false;
        const Vector3 originalStart = start;
        const Vector3 originalEnd = end;

        // start가 unwalkable이면 가장 가까운 walkable 셀로 스냅 (스무딩으로 살짝 파고든 경우 대응)
        if (gridMap->IsValid(startX, startZ) && !gridMap->IsWalkable(startX, startZ))
        {
            int snapX, snapZ;
            if (!FindNearestWalkable(gridMap, startX, startZ, snapX, snapZ))
            {
                result.success = false;
                return result;
            }
            startX = snapX;
            startZ = snapZ;
            usedSnap = true;
        }

        // end가 unwalkable이면 가장 가까운 walkable 셀로 스냅 (플레이어가 장애물 위에 있을 때 대응)
        if (gridMap->IsValid(endX, endZ) && !gridMap->IsWalkable(endX, endZ))
        {
            int snapX, snapZ;
            if (!FindNearestWalkable(gridMap, endX, endZ, snapX, snapZ))
            {
                result.success = false;
                return result;
            }
            endX = snapX;
            endZ = snapZ;
            usedEndSnap = true;
        }

        // 직선으로 이동 가능하면 바로 반환 (스냅된 start 기준으로 검사)
        Vector3 lineStart = usedSnap ? gridMap->GridToWorld(startX, startZ) : start;
        Vector3 lineEnd = usedEndSnap ? gridMap->GridToWorld(endX, endZ) : end;
        if (IsLineWalkable(lineStart, lineEnd))
        {
            result.success = true;
            result.path = { originalStart, lineEnd };
            result.rawPath = result.path;
            result.totalDistance = Vector3::Distance(originalStart, lineEnd);
            result.nodeCount = 2;
            result.optimizedNodeCount = 2;
            return result;
        }

        // A* 알고리즘 실행 (스냅된 start와 end 사용)
        result.rawPath = FindPathAStar(startX, startZ, endX, endZ);

        if (result.rawPath.empty())
        {
            result.success = false;
            return result;
        }

        result.nodeCount = static_cast<int>(result.rawPath.size());

        // String Pulling 적용
        result.path = StringPull(result.rawPath);
        result.optimizedNodeCount = result.nodeCount;

        //result.path = SmoothPath(result.path, 2);  // 2회 반복

        // 스냅했을 경우 첫 웨이포인트를 실제 현재 위치로 (들락날락 방지)
        if (usedSnap && !result.path.empty())
            result.path[0] = originalStart;

        // end를 스냅했을 경우 마지막 웨이포인트를 스냅된 위치로 유지 (장애물을 파고들지 않도록)
        // (이미 FindPathAStar에서 스냅된 end로 경로를 찾았으므로 마지막 점은 자동으로 스냅된 위치)

        // 거리 계산
        for (size_t i = 1; i < result.path.size(); ++i)
        {
            result.totalDistance += Vector3::Distance(result.path[i - 1], result.path[i]);
        }

        result.success = true;
        return result;
	}

    GridMap* PathfindingSystem::GetGridMap() const
    {
        if (m_components.empty())
        {
            return nullptr;
        }

        return m_components[0];
    }

    void PathfindingSystem::RegisterAgent(PathfindingAgent* component)
    {
        if (component->GetSystemIndex() != -1)
        {
            return;
        }

        component->SetSystemIndex(static_cast<std::int32_t>(m_agents.size()));
        m_agents.push_back(component);
    }

    void PathfindingSystem::UnregisterAgent(PathfindingAgent* component)
    {
        std::int32_t index = component->GetSystemIndex();

        if (index < 0 || index >= static_cast<std::int32_t>(m_agents.size()))
        {
            return;
        }

        PathfindingAgent* back = m_agents.back();
        m_agents[index] = back;
        m_agents.pop_back();

        back->SetSystemIndex(index);
        component->SetSystemIndex(-1);
    }

    void PathfindingSystem::Update()
    {
        for (auto agent : m_agents)
        {
            if (agent->IsActive())
            {
                agent->Update();
            }
        }
    }

	std::vector<Vector3> PathfindingSystem::FindPathAStar(int startX, int startZ, int endX, int endZ, float maxDistance)
	{
		std::vector<Vector3> path;

		const GridMap* gridMap = m_components[0];

        if (startX == endX && startZ == endZ)
        {
            path.push_back(gridMap->GridToWorld(startX, startZ));
            return path;
        }

        // 유효성 검사
        if (!gridMap->IsValid(startX, startZ) || !gridMap->IsValid(endX, endZ))
        {
            return path;
        }

        // 시작점이나 끝점이 walkable하지 않으면 실패
        if (!gridMap->IsWalkable(startX, startZ) || !gridMap->IsWalkable(endX, endZ))
        {
            return path;
        }

        // 최대 거리 제한 체크 (몬스터 AI용)
        if (maxDistance > 0.0f)
        {
            float directDistance = Heuristic(startX, startZ, endX, endZ) * gridMap->GetCellSize();
            if (directDistance > maxDistance)
            {
                return path;  // 너무 멀면 실패
            }
        }

        // 우선순위 큐 (fCost가 작은 것부터)
        std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;

        // 노드 관리용 맵 (그리드 좌표 -> PathNode)
        std::unordered_map<size_t, PathNode> nodeMap;

        // 닫힌 집합 (이미 탐색한 노드)
        std::unordered_set<size_t> closedSet;

        // 시작 노드 초기화
        size_t startHash = GetNodeHash(startX, startZ);
        PathNode startNode(startX, startZ);
        startNode.gCost = 0.0f;
        startNode.hCost = Heuristic(startX, startZ, endX, endZ);
        startNode.fCost = startNode.gCost + startNode.hCost;
        startNode.parent = nullptr;

        nodeMap[startHash] = startNode;
        openSet.push(startNode);

        // A* 메인 루프
        while (!openSet.empty())
        {
            // fCost가 가장 작은 노드 선택
            PathNode current = openSet.top();
            openSet.pop();

            size_t currentHash = GetNodeHash(current.x, current.z);

            // 이미 닫힌 집합에 있으면 스킵
            if (closedSet.find(currentHash) != closedSet.end())
            {
                continue;
            }

            // 닫힌 집합에 추가
            closedSet.insert(currentHash);

            // 목표 도달 체크
            if (current.x == endX && current.z == endZ)
            {
                // 경로 재구성 (역순으로 부모를 따라가며)
                std::vector<Vector3> reconstructedPath;
                PathNode* node = &nodeMap[currentHash];

                while (node != nullptr)
                {
                    reconstructedPath.push_back(gridMap->GridToWorld(node->x, node->z));

                    if (node->parent == nullptr)
                    {
                        break;
                    }

                    // 부모 노드 찾기
                    size_t parentHash = GetNodeHash(node->parent->x, node->parent->z);
                    auto it = nodeMap.find(parentHash);
                    if (it != nodeMap.end())
                    {
                        node = &it->second;
                    }
                    else
                    {
                        break;
                    }
                }

                // 역순이므로 뒤집기
                std::reverse(reconstructedPath.begin(), reconstructedPath.end());
                return reconstructedPath;
            }

            // 8방향 인접 노드 탐색
            for (int i = 0; i < 8; ++i)
            {
                int newX = current.x + s_directions[i][0];
                int newZ = current.z + s_directions[i][1];

                // 유효성 검사
                if (!gridMap->IsValid(newX, newZ))
                {
                    continue;
                }

                // Walkable 체크
                if (!gridMap->IsWalkable(newX, newZ))
                {
                    continue;
                }

                size_t neighborHash = GetNodeHash(newX, newZ);

                // 이미 닫힌 집합에 있으면 스킵
                if (closedSet.find(neighborHash) != closedSet.end())
                {
                    continue;
                }

                // 이동 비용 계산 (대각선인지 체크)
                bool isDiagonal = (s_directions[i][0] != 0 && s_directions[i][1] != 0);
                float moveCost = isDiagonal ? s_diagonalCost : s_straightCost;

                // 셀 비용 적용
                const GridCell& cell = gridMap->GetCell(newX, newZ);
                moveCost *= cell.cost;

                // 새로운 gCost 계산
                float newGCost = current.gCost + moveCost;

                // 노드가 맵에 없거나, 더 나은 경로를 찾았으면 업데이트
                auto it = nodeMap.find(neighborHash);
                if (it == nodeMap.end() || newGCost < it->second.gCost)
                {
                    PathNode neighbor(newX, newZ);
                    neighbor.gCost = newGCost;
                    neighbor.hCost = Heuristic(newX, newZ, endX, endZ);
                    neighbor.fCost = neighbor.gCost + neighbor.hCost;
                    neighbor.parent = &nodeMap[currentHash];  // 부모 설정

                    nodeMap[neighborHash] = neighbor;
                    openSet.push(neighbor);
                }
            }
        }

        // 경로를 찾지 못함
        return path;
	}

	float PathfindingSystem::Heuristic(int x1, int z1, int x2, int z2) const
	{
		int dx = x2 - x1;
		int dz = z2 - z1;
		return std::sqrt(static_cast<float>(dx * dx + dz * dz));
	}

	size_t PathfindingSystem::GetNodeHash(int x, int z) const
	{
		// 간단한 해시 함수 (x, z를 하나의 정수로 변환)
		return static_cast<size_t>(x) * 10000 + static_cast<size_t>(z);
	}

	bool PathfindingSystem::FindNearestWalkable(const GridMap* gridMap, int fromX, int fromZ, int& outX, int& outZ) const
	{
		if (!gridMap->IsValid(fromX, fromZ))
			return false;
		if (gridMap->IsWalkable(fromX, fromZ))
		{
			outX = fromX;
			outZ = fromZ;
			return true;
		}

		std::queue<std::pair<int, int>> q;
		std::unordered_set<size_t> visited;
		q.push({ fromX, fromZ });
		visited.insert(GetNodeHash(fromX, fromZ));

		while (!q.empty())
		{
			int x = q.front().first;
			int z = q.front().second;
			q.pop();

			for (int i = 0; i < 8; ++i)
			{
				int nx = x + s_directions[i][0];
				int nz = z + s_directions[i][1];
				if (!gridMap->IsValid(nx, nz))
					continue;
				size_t h = GetNodeHash(nx, nz);
				if (visited.count(h))
					continue;
				visited.insert(h);
				if (gridMap->IsWalkable(nx, nz))
				{
					outX = nx;
					outZ = nz;
					return true;
				}
				q.push({ nx, nz });
			}
		}
		return false;
	}

    std::vector<Vector3> PathfindingSystem::StringPull(const std::vector<Vector3>& rawPath)
    {
        const GridMap* gridMap = m_components[0];

        if (rawPath.size() <= 2)
        {
            return rawPath;
        }

        std::vector<Vector3> optimized;
        optimized.push_back(rawPath[0]);

        int current = 0;
        while (current < static_cast<int>(rawPath.size()) - 1)
        {
            int furthest = current + 1;

            // current에서 가장 멀리 떨어진 직선 이동 가능한 노드 찾기
            for (int i = static_cast<int>(rawPath.size()) - 1; i > current; i--)
            {
                if (IsLineWalkable(rawPath[current], rawPath[i]))
                {
                    furthest = i;
                    break;
                }
            }

            optimized.push_back(rawPath[furthest]);
            current = furthest;
        }

        return optimized;
    }

    bool PathfindingSystem::IsLineWalkable(const Vector3& start, const Vector3& end)
    {
        const GridMap* gridMap = m_components[0];

        // 시작점과 끝점을 그리드 좌표로 변환
        int startX, startZ, endX, endZ;
        gridMap->WorldToGrid(start, startX, startZ);
        gridMap->WorldToGrid(end, endX, endZ);

        // Bresenham 알고리즘으로 직선상의 모든 셀 체크
        int dx = std::abs(endX - startX);
        int dz = std::abs(endZ - startZ);
        int sx = (startX < endX) ? 1 : -1;
        int sz = (startZ < endZ) ? 1 : -1;
        int err = dx - dz;

        int x = startX;
        int z = startZ;

        while (true)
        {
            // 현재 셀이 walkable한지 체크
            if (!gridMap->IsWalkable(x, z))
            {
                return false;
            }

            // 목표 도달
            if (x == endX && z == endZ)
            {
                break;
            }

            int e2 = 2 * err;
            if (e2 > -dz)
            {
                err -= dz;
                x += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                z += sz;
            }
        }

        return true;
    }

    std::vector<Vector3> PathfindingSystem::SmoothPath(const std::vector<Vector3>& path, int iterations)
    {
        if (path.size() <= 2)
            return path;

        std::vector<Vector3> smoothed = path;

        // 여러 번 반복하여 부드럽게 만듦
        for (int iter = 0; iter < iterations; ++iter)
        {
            std::vector<Vector3> newPath;
            newPath.push_back(smoothed[0]);  // 시작점은 유지

            // 중간 점들을 부드럽게 조정
            for (size_t i = 1; i < smoothed.size() - 1; ++i)
            {
                // 이전 점, 현재 점, 다음 점의 평균 위치 계산
                Vector3 prev = smoothed[i - 1];
                Vector3 curr = smoothed[i];
                Vector3 next = smoothed[i + 1];

                // 가중 평균 (현재 점에 더 가중치)
                Vector3 smoothedPos = prev * 0.25f + curr * 0.5f + next * 0.25f;

                // 스무딩된 위치가 walkable한지 확인
                // 이전 점과 다음 점 사이의 직선이 walkable하면 스무딩된 위치 사용
                if (IsLineWalkable(prev, next))
                {
                    // 스무딩된 위치를 walkable 영역으로 제한
                    // (간단히 스무딩된 위치 사용)
                    newPath.push_back(smoothedPos);
                }
                else
                {
                    // 스무딩 불가능하면 원래 위치 유지
                    newPath.push_back(curr);
                }
            }

            newPath.push_back(smoothed.back());  // 끝점은 유지
            smoothed = newPath;
        }

        return smoothed;
    }
}