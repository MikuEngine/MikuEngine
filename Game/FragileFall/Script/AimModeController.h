#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>

namespace engine
{
    class UIImage;
    class RectTransform;
    class Canvas;
}

namespace game
{
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
            AimFiring,
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
        /** 이동 중일 때만 Y 보정 적용. 서서 쏠 때는 0, 걸을 때만 m_aimYOffsetWhenMoving 사용 */
        void SetMoving(bool moving) { m_isMoving = moving; }

        // 플레이어에서 에임포인터 방향을 얻는 함수
        engine::Vector3 GetDirectionFrom(const engine::Vector3& fromPosition) const;

        // 현재 에임포인터 월드 위치
        const engine::Vector3& GetWorldPosition() const { return m_worldPosition; }

    private:
        AimMode m_baseMode = AimMode::Pointer;
        bool    m_combatAimEnabled = false;
        bool    m_paused = false;

        AimMode ComputeEffectiveMode() const;
        void SetCursorTexture(AimCursorState state);

        // DebugIndex
        int m_debugIndex = 0;

        // World Aim
        engine::Vector3 m_worldPosition;  // 마우스 월드 좌표

        // UI 커서 컴포넌트 (씬의 Canvas 오브젝트에서 관리)
        engine::GameObject* m_cursorObject = nullptr;
        engine::UIImage* m_cursorImage = nullptr;
        engine::RectTransform* m_cursorRect = nullptr;
        engine::Canvas* m_canvas = nullptr;

        // Canvas 설정
        std::string m_canvasObjectName = "AimPointerCanvas";  // 씬에서 찾을 Canvas 오브젝트 이름

        // 커서 설정
        AimCursorState m_cursor = AimCursorState::Default;
        std::unordered_map<AimCursorState, std::string> m_cursorTextures;
        std::array<std::string, (int)AimCursorState::Count> m_cursorTexByState;
        std::array<engine::Vector2, (int)AimCursorState::Count> m_cursorPivotByState;

        engine::Vector2 m_cursorSize{ 30.0f, 30.0f };
        engine::Vector2 m_cursorPivot{ 0.0f, 0.0f };

        // 월드 좌표 계산 설정
        float m_targetPlaneY = 1.5f;  // 레이캐스트 대상 평면의 Y 높이 (총알 발사 높이)
        /** 서서 쏠 때는 0, 걸을 때만 적용되는 에임 Y 보정 (양수=위, 음수=아래). 서서 쏠 때는 Target Plane Y로 조정 */
        float m_aimYOffsetWhenMoving = 0.0f;
        bool m_isMoving = false;  // 플레이어가 이동 중인지 (외부에서 SetMoving으로 설정)


    private:
        void EnsureUICursor();
        void UpdateWorldPositionFromMouse(const engine::Vector2& mousePos);

        void SetCursorVisible(bool visible);
        AimCursorState ComputeDesiredCursorState(AimMode mode) const;

    private:
        void TickWorldAim(const engine::Vector2& mousePx, AimMode mode);  // 월드계산(기존 로직 호출)
        void TickUICursor(const engine::Vector2& mousePx, AimMode mode);  // UI 표시
    };
}