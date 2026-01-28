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

    // ═══════════════════════════════════════════════════════════════
    // ExecutionIndicatorManager - Fragile 몬스터 처형 인디케이터 관리
    // 
    // 기능:
    //   - Fragile 상태의 몬스터 위에 마우스 호버 시 인디케이터 표시
    //   - 마우스 우클릭 시 Y축 회전 애니메이션 후 몬스터 Dead 상태 전이
    // 
    // 사용법:
    //   - 씬에 빈 GameObject 생성 후 이 스크립트 추가
    //   - ExcutionIndicator 프리팹 필요
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
        // 런타임 상태
        // ─────────────────────────────────────────────
        engine::Ptr<engine::Camera> m_mainCamera;
        engine::Ptr<engine::GameObject> m_indicatorInstance;
        engine::Ptr<engine::Transform> m_indicatorTransform;
        engine::Ptr<MonsterScript> m_hoveredMonster;
        
        // 회전 애니메이션 상태
        bool m_isExecuting = false;
        float m_executionTimer = 0.0f;
        engine::Quaternion m_initialRotation;
        engine::Ptr<MonsterScript> m_executingMonster;

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
        
        // 마우스 호버 처리
        MonsterScript* GetFragileMonsterUnderMouse();
        
        // 처형 애니메이션
        void StartExecution(MonsterScript* monster);
        void UpdateExecution(float deltaTime);
        void FinishExecution();
    };
}
