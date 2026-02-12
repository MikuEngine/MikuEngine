#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace engine
{
    class Camera;
    class GameObject;
    class Transform;
}

namespace game
{
    class MonsterScript;
    class StageClearExecutionTarget;
    class PlayerControllerScript;
    class ExecutionSlowScript;
    class CameraEffectScript;
    class AimModeController;
    class UIMessageQueue;

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
        float m_linePixelsPerMeter = 50.0f; // 1m당 픽셀 수 (점선 밀도 조절)
        float m_lineMinPixels = 50.0;     // 최소 드로우 픽셀 크기
        float m_linePixelStep = 50.0f;      // 증가 단위 픽셀 (계단식 증가)
        
        // ─────────────────────────────────────────────
        // 반사 화살표 설정 (빅탄 처형 시에만 표시)
        // ─────────────────────────────────────────────
        std::string m_reflectIndicatorPrefabName = "Execution_Reflect_Indicator";
        float m_reflectIndicatorHeight = 1.5f;   // 화살표 Y 높이
        float m_reflectIndicatorOffset = 1.5f;   // 빅탄 중점에서 방향 연장선상 오프셋
        
        // ─────────────────────────────────────────────
        // 방향 기반 오프셋 스케일링
        // ─────────────────────────────────────────────
        float m_offsetScaleZ = 1.0f;  // Z축 평행 방향일 때 오프셋 배율 (0~2)
        float m_offsetScaleX = 1.0f;  // X축 평행 방향일 때 오프셋 배율 (0~2)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        engine::Ptr<engine::Camera> m_mainCamera;
        engine::Ptr<engine::GameObject> m_indicatorInstance;
        engine::Ptr<engine::Transform> m_indicatorTransform;
        engine::Ptr<engine::GameObject> m_hoveredGameObject;
        
        // 플레이어 참조
        engine::Ptr<PlayerControllerScript> m_player;
        
        // AimModeController 참조 (커서 상태 업데이트용)
        engine::Ptr<class AimModeController> m_aimController;
        
        // 라인 인스턴스
        engine::Ptr<engine::GameObject> m_lineInstance;
        engine::Ptr<engine::Transform> m_lineTransform;
        
        // 반사 화살표 인스턴스 (빅탄 처형 시에만 표시)
        engine::Ptr<engine::GameObject> m_reflectIndicatorInstance;
        engine::Ptr<engine::Transform> m_reflectIndicatorTransform;
        
        // ─────────────────────────────────────────────
        // 처형 이펙트 설정
        // ─────────────────────────────────────────────
        std::string m_effectPrefabName = "ExcutionEffectSprite";  // 처형 이펙트 프리팹
        float m_effectDuration = 0.2f;            // 이펙트 지속 시간 (초)
        float m_effectScaleMultiplier = 1.5f;     // 이펙트 최종 스케일 배율
        float m_monsterDeathDelay = 0.05f;        // 텔레포트 후 몬스터 Death까지 대기 시간 (초)
        /** 처형 애니메이션 재생 비율(0~1) 도달 시 순간이동·처형 실행. 0.5 = 50% 재생 후 */
        float m_executionTeleportAtNormalizedTime = 0.5f;

        /** 처형 애니 최소 재생 시간(초). 이 시간이 지나야 비율 조건과 함께 처형 발동 (가까운 적이어도 애니가 잠깐은 보이게) */
        float m_executionMinDuration = 0.4f;

        /** 처형 순간이동 시 잔상(Afterimage) 구간에 넣을 슬라이스 수. 0이면 잔상 미사용. 클수록 잔상이 길고 촘촘함 */
        size_t m_teleportAfterimageNumSlices = 32;

        /** true면 AfterimageRenderer 기본 trail gradient 사용, false면 아래 값으로 오버라이드 */
        bool m_teleportAfterimageUseDefaultGradient = false;

        /** 텔레포트 잔상용 trail gradient 오버라이드 (0~1). 1에 가까울수록 구간 전체 비슷한 알파, 작을수록 앞쪽이 빨리 흐려짐. UseDefaultGradient가 true면 무시 */
        float m_teleportAfterimageTrailGradient = 0.97f;

        // 처형 런타임 상태
        engine::Ptr<engine::GameObject> m_executingGameObject;

        /** 제자리에서 처형 애니 재생 중, 이 비율 도달 시 순간이동 대기 */
        bool m_isWaitingForExecutionAnim = false;
        /** 처형 애니 대기 시작 시각 (UnscaledTime). 최소 재생 시간 체크용 */
        float m_executionAnimWaitStartTime = 0.0f;

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

        // 처형 킬 메시지 출력용
        engine::Ptr<UIMessageQueue> m_uiMessageQueue;
        std::vector<std::string> m_killMessageKeys;
        std::unordered_map<std::string, int> m_killMessageUseCount;
        std::string m_lastKillMessageKey;
        bool m_wasPlayerDead = false;
        std::string m_lastSceneName;

        // 커서 처형 판정 래치(히스테리시스)
        // - true -> false 전환만 지연하여 1~2프레임 판정 흔들림을 흡수
        int m_executionTargetOffLatchFrames = 2;
        int m_executionTargetLostFrames = 0;
        bool m_executionTargetLatched = false;

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
        
        // 반사 화살표 관련 (빅탄 처형 전용)
        void CreateReflectIndicatorInstance();
        void DestroyReflectIndicatorInstance();
        void UpdateReflectIndicator(engine::GameObject* projectile);
        void ShowReflectIndicator();
        void HideReflectIndicator();
        
        // 방향 기반 오프셋 스케일 계산
        float GetDirectionalOffsetScale(const engine::Vector3& direction) const;
        
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

        void RefreshKillMessageKeys();
        void ResetKillMessageStats();
        std::string BuildBalancedKillMessageKey();
        void PushRandomKillMessage();
        void UpdateExecutionTargetLatch(bool isOnExecutionTargetRaw);
    };
}
