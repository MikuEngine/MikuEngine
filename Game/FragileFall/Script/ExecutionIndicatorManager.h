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
    class ExecutionSlowScript;
    class CameraEffectScript;

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
        engine::Ptr<engine::GameObject> m_hoveredGameObject;
        
        // 플레이어 참조
        engine::Ptr<PlayerControllerScript> m_player;
        
        // 라인 인스턴스
        engine::Ptr<engine::GameObject> m_lineInstance;
        engine::Ptr<engine::Transform> m_lineTransform;
        
        // ─────────────────────────────────────────────
        // 처형 이펙트 설정
        // ─────────────────────────────────────────────
        std::string m_effectPrefabName = "ExcutionEffectSprite";  // 처형 이펙트 프리팹
        float m_effectDuration = 0.2f;            // 이펙트 지속 시간 (초)
        float m_effectScaleMultiplier = 1.5f;     // 이펙트 최종 스케일 배율
        float m_monsterDeathDelay = 0.05f;        // 텔레포트 후 몬스터 Death까지 대기 시간 (초)

        // 처형 런타임 상태
        engine::Ptr<engine::GameObject> m_executingGameObject;

        // 몬스터 Death 타이머
        bool m_isWaitingForDeath = false;
        float m_deathTimer = 0.0f;

        // 플레이어 Idle 전이 대기
        bool m_isWaitingForIdle = false;
        int m_idleWaitFrames = 0;

        // ─────────────────────────────────────────────
        // 트리거 변경 대기 상태
        // ─────────────────────────────────────────────
        bool m_isWaitingForTrigger = false;    // 트리거 변경 후 프레임 대기 중
        int m_triggerWaitFrames = 0;           // 대기한 프레임 수
        int m_triggerWaitFramesRequired = 1;   // 필요한 대기 프레임 수 (향후 확장 가능)

        // ─────────────────────────────────────────────
        // 슬로우 효과 스크립트 참조
        // ─────────────────────────────────────────────
        engine::Ptr<ExecutionSlowScript> m_slowScript;
        
        // ─────────────────────────────────────────────
        // 카메라 이펙트 스크립트 참조
        // ─────────────────────────────────────────────
        engine::Ptr<CameraEffectScript> m_cameraEffectScript;

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
        void UpdateLine(const engine::Vector3& targetPos);
        void ShowLine();
        void HideLine();
        
        // 마우스 호버 처리
        engine::GameObject* GetFragileMonsterUnderMouse();
        
        // 처형 시퀀스
        void StartExecution(engine::GameObject* target);
        void UpdateTriggerWait();              // 트리거 변경 후 프레임 대기
        void PerformTeleport();                // 몬스터 위치로 순간이동
        void UpdateIdleWait();                 // Idle 전이 대기
        void UpdateDeathTimer(float deltaTime);// 몬스터 Death 타이머
        void TriggerMonsterDeath();            // 몬스터 Death 처리
        void SpawnExecutionEffect(const engine::Vector3& position);  // 처형 이펙트 생성
        
        // 충돌 검사 (향후 확장용)
        void SetMonsterColliderTrigger(engine::GameObject* monster, bool isTrigger);
        bool IsMonsterColliderTrigger(engine::GameObject* monster) const;  // 트리거 상태 확인
        bool IsPathClearForTeleport() const;   // 향후: 근처 충돌 가능 콜라이더 검사
        void CancelExecution();                // 처형 취소 (트리거 확인 실패 시)
        
        // 거리 계산 헬퍼
        float GetDistanceToMonster(engine::GameObject* target) const;
        bool IsMonsterInExecutionRange(engine::GameObject* target) const;
    };
}
