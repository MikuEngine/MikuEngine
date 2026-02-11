#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <vector>

namespace engine
{
    class RectTransform;
    class Canvas;
}

namespace game
{
    class PlayerControllerScript;

    class AimModeController :
        public engine::Script<AimModeController>
    {
        REGISTER_SCRIPT(AimModeController, Script)

    public:
        enum class AimMode
        {
            Pointer,     // 기본 UI 포인터(메뉴/로비 등)
            CombatAim,   // 전투 조준(월드 에임 + 레티클/커서)
        };

    private:
        enum class AimCursorState
        {
            Default,
            Clicked,

            AimIdle,
            AimOnEnemy,
            AimExecute,

            Count
        };

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    public:
        void SetCombatAimEnabled(bool enabled); // 전투모드 on/off
        void SetPaused(bool paused);            // 퍼즈상태

        AimMode GetEffectiveMode() const { return ComputeEffectiveMode(); }

    public:
        /** 이동 중인지 설정 (PlayerControllerScript에서 전달) */
        void SetMoving(bool moving) { m_isMoving = moving; }
        
        /** 처형 대상 위에 커서가 있는지 설정 (ExecutionIndicatorManager에서 호출) */
        void SetOnExecutionTarget(bool isOnTarget) { m_isOnExecutionTarget = isOnTarget; }
        /** 일반 적 대상 위에 커서가 있는지 설정 */
        void SetOnEnemyTarget(bool isOnEnemy) { m_isOnEnemyTarget = isOnEnemy; }
        /** 처형 진행 중 커서를 강제로 Execute로 유지 (처형 시작~상태 종료) */
        void SetExecutionInProgress(bool inProgress) { m_isExecutionInProgress = inProgress; }

        // 플레이어에서 에임포인터 방향을 얻는 함수
        engine::Vector3 GetDirectionFrom(const engine::Vector3& fromPosition) const;

        // 현재 에임포인터 월드 위치
        const engine::Vector3& GetWorldPosition() const { return m_worldPosition; }
        // 현재 마우스 레이를 임의의 Y 평면에 투영한 교점
        bool TryGetMouseRayPlaneIntersection(float planeY, engine::Vector3& outWorldPos) const;

    private:
        AimMode m_baseMode = AimMode::Pointer;
        bool    m_combatAimEnabled = false;
        bool    m_isOnExecutionTarget = false;
        bool    m_isOnEnemyTarget = false;
        bool    m_isExecutionInProgress = false;
        bool    m_paused = false;
        float   m_onEnemyRayMaxDistance = 1000.0f;

        AimMode ComputeEffectiveMode() const;
        void ApplyCursorState(AimCursorState state);

        // DebugIndex
        int m_debugIndex = 0;

        // World Aim
        engine::Vector3 m_worldPosition;  // 마우스 월드 좌표

        // UI 커서 컴포넌트 (씬의 Canvas 오브젝트에서 관리)
        engine::Canvas* m_canvas = nullptr;
        engine::GameObject* m_cursorAimObject = nullptr;
        engine::GameObject* m_cursorExecutionObject = nullptr;
        engine::GameObject* m_cursorDefaultObject = nullptr;
        engine::GameObject* m_cursorClickObject = nullptr;
        engine::GameObject* m_cursorOnEnemyObject = nullptr;
        engine::RectTransform* m_cursorAimRect = nullptr;
        engine::RectTransform* m_cursorExecutionRect = nullptr;
        engine::RectTransform* m_cursorDefaultRect = nullptr;
        engine::RectTransform* m_cursorClickRect = nullptr;
        engine::RectTransform* m_cursorOnEnemyRect = nullptr;

        // 오브젝트 이름은 직렬화하지 않고 코드 상수로 고정한다.
        static constexpr const char* kCanvasObjectName = "AimPointerCanvas";
        static constexpr const char* kCursorAimName = "AimCursor";
        static constexpr const char* kCursorExecutionName = "AimCursor_Execution";
        static constexpr const char* kCursorDefaultName = "AimCursor_Default";
        static constexpr const char* kCursorClickName = "AimCursor_Click";
        static constexpr const char* kCursorOnEnemyName = "AimCursor_OnEnemy";

        // 커서 설정
        AimCursorState m_cursor = AimCursorState::Default;
        engine::Vector2 m_cursorSize{ 50.0f, 50.0f };

        // 월드 좌표 계산 설정
        float m_targetPlaneY = 1.7f;  // 레이캐스트 대상 평면의 Y 높이 (총알 발사 높이 근사값)
        float m_aimYOffsetWhenMoving = -0.4f; // 이동 중 추가 Y 보정 (예: 1.7 + (-0.4) = 1.3)
        bool m_isMoving = false;
    
    public:
        // 처형 완료 후 타이머 (ExecutionIndicatorManager가 접근)
        float m_postExecutionTimer = 0.0f;
        float m_postExecutionDuration = 0.5f;  // 처형 완료 후 Execute 커서 유지 시간 (초)


    private:
        void EnsureUICursor();
        void EnsurePulseBindings();
        void CollectPulseSprites(engine::GameObject* root, std::vector<engine::RectTransform*>& outSprites, std::vector<engine::Vector2>& outBasePositions, std::vector<engine::Vector2>& outDirections);
        void OnPlayerFired();
        void UpdateCursorPulse(float deltaTime);
        void ApplyCursorPulseToSprites();
        void ResetCursorPulseSprites();
        void EnsureExecutionIntroBindings();
        void StartExecutionIntro();
        void UpdateExecutionIntro(float deltaTime);
        void ApplyExecutionIntroPose(float alpha);
        void ResetExecutionIntroPose();
        bool TryBuildMouseRayFromScreen(const engine::Vector2& mousePos, engine::Vector3& outRayOrigin, engine::Vector3& outRayDir) const;
        void UpdateWorldPositionFromMouse(const engine::Vector2& mousePos);
        void UpdateOnEnemyByRaycast(const engine::Vector2& mousePos, AimMode mode);
        AimCursorState ComputeDesiredCursorState(AimMode mode) const;

    private:
        void TickWorldAim(const engine::Vector2& mousePx, AimMode mode);  // 월드계산(기존 로직 호출)
        void TickUICursor(const engine::Vector2& mousePx, AimMode mode);  // UI 표시

    private:
        PlayerControllerScript* m_playerController = nullptr;
        bool m_fireCallbackRegistered = false;
        bool m_pulseSpritesInitialized = false;

        std::vector<engine::RectTransform*> m_pulseAimSprites;
        std::vector<engine::RectTransform*> m_pulseOnEnemySprites;
        std::vector<engine::Vector2> m_pulseAimBasePositions;
        std::vector<engine::Vector2> m_pulseOnEnemyBasePositions;
        std::vector<engine::Vector2> m_pulseAimDirections;
        std::vector<engine::Vector2> m_pulseOnEnemyDirections;

        bool m_enableCursorPulse = true;
        float m_pulseMaxDistance = 10.0f;
        float m_pulsePeakTimeScale = 0.5f;
        float m_pulseDurationPerFR = 1.0f;

        bool m_isPulsePlaying = false;
        bool m_isPulseMovingOutward = false;
        float m_pulseCurrent = 0.0f;
        float m_pulseOutDuration = 0.05f;
        float m_pulseBackDuration = 0.05f;

        bool m_executionIntroInitialized = false;
        bool m_executionIntroPlaying = false;
        bool m_prevOnExecutionTarget = false;
        float m_executionIntroTimer = 0.0f;
        float m_executionIntroScale = 1.5f;
        float m_executionIntroDistancePx = 10.0f;
        float m_executionIntroDuration = 0.5f;

        std::vector<engine::RectTransform*> m_executionIntroSprites;
        std::vector<engine::Vector2> m_executionIntroBasePositions;
        std::vector<engine::Vector3> m_executionIntroBaseScales;
        std::vector<engine::Vector2> m_executionIntroDirections;
    };
}