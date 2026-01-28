#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class PathfindingAgent :
        public Component
    {
        REGISTER_COMPONENT(PathfindingAgent, Component)

    private:
        std::vector<Vector3> m_path;
        int m_currentPathIndex = 0;
        bool m_hasPath = false;
        bool m_pathRequested = false;
        Vector3 m_targetDestination;

        // 몬스터 AI 최적화용
        float m_pathUpdateInterval = 1.0f;
        float m_pathUpdateTimer = 0.0f;
        float m_waypointReachDistance = 0.5f;
        Vector3 m_lastTargetPos;
        float m_targetMoveThreshold = 1.0f;

        Vector3 m_currentWaypoint;
        bool m_hasWaypoint = false;

    public:
        ~PathfindingAgent();

    public:
        void Initialize() override;

        // 경로 요청
        void RequestPath(const Vector3& destination);

        // 경로 상태 조회
        bool HasPath() const;
        bool IsPathComplete() const;
        const std::vector<Vector3>& GetPath() const;

        // Waypoint 조회
        Vector3 GetCurrentWaypoint() const;
        Vector3 GetNextWaypoint() const;
        void AdvanceToNextWaypoint();

        // 몬스터 AI용 (deltaTime 기반 - 기존 방식, 1프레임 지연 있음)
        void UpdatePathfinding(float deltaTime, const Vector3& targetPosition);
        
        // 몬스터 AI용 (FixedUpdate 기반 - 즉시 계산, 지연 없음)
        // Physics와 동기화된 경로 업데이트가 필요할 때 사용
        void UpdatePathfindingFixed(float fixedDeltaTime, const Vector3& targetPosition);
        
        // 즉시 경로 계산 (요청과 동시에 FindPath 실행)
        void RequestPathImmediate(const Vector3& destination);
        
        bool GetCurrentWaypoint(Vector3& outWaypoint) const;
        bool IsWaypointReached(const Vector3& currentPos, float threshold = -1.0f) const;

        // 경로 초기화
        void ClearPath();

        // 설정
        void SetPathUpdateInterval(float interval);
        void SetWaypointReachDistance(float distance);
        void SetTargetMoveThreshold(float threshold);

    public:
        void Update();
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    private:
        void ProcessPathRequest();

        friend class PathfindingSystem;
    };
}