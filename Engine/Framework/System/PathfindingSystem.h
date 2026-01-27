#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Pathfinding/GridMap.h"

namespace engine
{
	struct PathNode
	{
		int x = 0;
		int z = 0;
		float gCost = 0.0f;
		float hCost = 0.0f;
		float fCost = 0.0f;
		PathNode* parent = nullptr;

		PathNode() {}
		PathNode(int x, int z)
			: x{ x }, z{ z }
		{

		}

		bool operator>(const PathNode& other) const
		{
			return fCost > other.fCost;
		}
	};

	struct PathResult
	{
		bool success = false;
		std::vector<Vector3> path;           // 최종 경로 (직선화됨)
		std::vector<Vector3> rawPath;        // 원본 A* 경로 (디버그용)
		float totalDistance = 0.0f;
		int nodeCount = 0;                   // 원본 노드 수
		int optimizedNodeCount = 0;          // 최적화 후 노드 수
	};

	class PathfindingAgent;

	class PathfindingSystem :
		public System<GridMap>
	{
	private:
		static constexpr int s_directions[8][2]{
		   {  0,  1 },  // 북
		   {  1,  1 },  // 북동
		   {  1,  0 },  // 동
		   {  1, -1 },  // 남동
		   {  0, -1 },  // 남
		   { -1, -1 },  // 남서
		   { -1,  0 },  // 서
		   { -1,  1 }   // 북서
		};

		// 대각선 이동 비용
		static constexpr float s_diagonalCost = 1.41421356f;  // √2
		static constexpr float s_straightCost = 1.0f;

		std::vector<PathfindingAgent*> m_agents;

	public:
		// 최종 경로 찾기 (A* + String Pulling)
		PathResult FindPath(const Vector3& start, const Vector3& end);
		GridMap* GetGridMap() const;

	public:
		virtual void RegisterAgent(PathfindingAgent* component);
		virtual void UnregisterAgent(PathfindingAgent* component);

		void Update();

	private:
		// A* 알고리즘
		std::vector<Vector3> FindPathAStar(
			int startX,
			int startZ,
			int endX,
			int endZ,
			float maxDistance = -1.0f  // -1이면 제한 없음
		);

		// 휴리스틱 함수 (유클리드 거리)
		float Heuristic(int x1, int z1, int x2, int z2) const;

		// 노드 비교용 해시 함수
		size_t GetNodeHash(int x, int z) const;

		// String Pulling
		std::vector<Vector3> StringPull(const std::vector<Vector3>& rawPath);

		// Line Walkability 체크 (Grid 기반)
		bool IsLineWalkable(const Vector3& start, const Vector3& end);

		std::vector<Vector3> SmoothPath(const std::vector<Vector3>& path, int iterations = 2);

		// start가 unwalkable일 때 가장 가까운 walkable 셀 탐색 (BFS)
		bool FindNearestWalkable(const GridMap* gridMap, int fromX, int fromZ, int& outX, int& outZ) const;
	};
}