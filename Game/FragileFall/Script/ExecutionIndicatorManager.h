#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>

namespace engine
{
    class Camera;
    class GameObject;
    class Transform;
}

namespace game
{
    class MonsterScript;
    class PlayerControllerScript;

    // ═══════════════════════════════════════════════════════════════
    // ExecutionIndicatorManager - Fragile 몬스터 처형 인디케이터 관리
    // 
    // 기능:
    //   - Fragile 상태의 몬스터 위에 마우스 호버 시 인디케이터 표시
    //   - 플레이어-몬스터 연결 라인 표시
    //   - 마우스 우클릭 시 Y축 회전 애니메이션 후 몬스터 Dead 상태 전이
    // 
    // 사용법:
    //   - 씬에 빈 GameObject 생성 후 이 스크립트 추가
    //   - ExcutionIndicator 프리팹 필요 (자식으로 IndicatorLine 포함)
    // ═══════════════════════════════════════════════════════════════
    class ExecutionIndicatorManager :
        public engine::Script<ExecutionIndicatorManager>
    {
        REGISTER_SCRIPT(ExecutionIndicatorManager, Script)

    private:
        // ─────────────────────────────────────────────
        // 설정
        // ─────────────────────────────────────────────
        std::string m_indicatorPrefabName = "ExcutionIndicator";
        float m_rotationDuration = 1.5f;         // 회전 애니메이션 시간 (초)
        float m_raycastMaxDistance = 1000.0f;    // 레이캐스트 최대 거리
        engine::Vector3 m_indicatorOffset{ 0.0f, 0.1f, 0.0f };  // 몬스터 위 오프셋

        // ─────────────────────────────────────────────
        // 라인 설정
        // ─────────────────────────────────────────────
        std::string m_linePrefabName = "IndicatorLine";  // 라인 프리팹 이름
        float m_linePlayerOffset = 0.5f;    // 플레이어-라인 간 오프셋 거리
        float m_lineMonsterOffset = 0.5f;   // 라인-몬스터 간 오프셋 거리
        float m_lineBaseLength = 1.0f;      // 스케일 1일 때 라인의 월드 길이
        float m_lineHeight = 0.5f;          // 라인의 Y 높이

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        engine::Ptr<engine::Camera> m_mainCamera;
        engine::Ptr<engine::GameObject> m_indicatorInstance;
        engine::Ptr<engine::Transform> m_indicatorTransform;
        engine::Ptr<MonsterScript> m_hoveredMonster;
        
        // 플레이어 참조
        engine::Ptr<PlayerControllerScript> m_player;
        
        // 라인 인스턴스
        engine::Ptr<engine::GameObject> m_lineInstance;
        engine::Ptr<engine::Transform> m_lineTransform;
        
        // 회전 애니메이션 상태
        bool m_isExecuting = false;
        float m_executionTimer = 0.0f;
        engine::Quaternion m_initialRotation;
        engine::Ptr<MonsterScript> m_executingMonster;

        // ─────────────────────────────────────────────
        // 대시 순간이동 설정
        // ─────────────────────────────────────────────
        float m_dashDistance = 2.0f;           // 한 번에 이동하는 거리
        float m_dashInterval = 0.15f;          // 대시 간격 (초)
        float m_finalDashThreshold = 2.5f;     // 이 거리 이하면 최종 도달

        // 대시 런타임 상태
        bool m_isDashing = false;
        float m_dashTimer = 0.0f;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        // ─────────────────────────────────────────────
        // 내부 헬퍼 함수
        // ─────────────────────────────────────────────
        void CreateIndicatorInstance();
        void DestroyIndicatorInstance();
        void ShowIndicator(engine::Transform* targetTransform);
        void HideIndicator();
        
        // 라인 관련
        void CreateLineInstance();
        void DestroyLineInstance();
        void UpdateLine(const engine::Vector3& monsterPos);
        void ShowLine();
        void HideLine();
        
        // 마우스 호버 처리
        MonsterScript* GetFragileMonsterUnderMouse();
        
        // 처형 애니메이션
        void StartExecution(MonsterScript* monster);
        void UpdateExecution(float deltaTime);
        void FinishExecution();
        
        // 대시 순간이동
        void UpdateDash(float deltaTime);
        void PerformDash();
        void FinishDash();
        
        // 거리 계산 헬퍼
        float GetDistanceToMonster(MonsterScript* monster) const;
        bool IsMonsterInExecutionRange(MonsterScript* monster) const;
    };
}
