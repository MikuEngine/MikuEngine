#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>

namespace engine
{
    class Camera;
    class Canvas;
    class UIText;
    class RectTransform;
    class GameObject;
}

namespace game
{
    class BaseControllerScript;
    class MonsterScript;

    // ═══════════════════════════════════════════════════════════════
    // StateTextRenderer - FSM 상태 텍스트 렌더러
    // 
    // 기능:
    //   - 씬의 모든 BaseControllerScript를 찾아서 추적
    //   - 각 오브젝트 위에 LogicFSM의 현재 상태를 표시
    //   - 절두체 밖이면 텍스트 숨김
    // 
    // 사용법:
    //   - 이 스크립트가 붙은 오브젝트에 Canvas 컴포넌트 필요
    //   - "StateText" 프리팹 필요 (UIText 컴포넌트 포함)
    // ═══════════════════════════════════════════════════════════════
    class StateTextRenderer :
        public engine::Script<StateTextRenderer>
    {
        REGISTER_SCRIPT(StateTextRenderer, Script)

    private:
        // ─────────────────────────────────────────────
        // 추적 대상과 UI 매핑 (Ptr로 댕글링 방지)
        // ─────────────────────────────────────────────
        struct TrackedObject
        {
            engine::Ptr<BaseControllerScript> controller;
            
            // 상태 텍스트
            engine::Ptr<engine::GameObject> textObject;
            engine::Ptr<engine::UIText> uiText;
            engine::Ptr<engine::RectTransform> rectTransform;
            
            // 체력 텍스트 (MonsterScript 전용)
            engine::Ptr<engine::GameObject> hpTextObject;
            engine::Ptr<engine::UIText> hpUiText;
            engine::Ptr<engine::RectTransform> hpRectTransform;
        };
        std::vector<TrackedObject> m_trackedObjects;

        // ─────────────────────────────────────────────
        // 캐시된 컴포넌트/참조 (Ptr로 댕글링 방지)
        // ─────────────────────────────────────────────
        engine::Ptr<engine::Camera> m_mainCamera;
        engine::Ptr<engine::Canvas> m_canvas;
        engine::Ptr<engine::RectTransform> m_parentRT;

        // ─────────────────────────────────────────────
        // 설정
        // ─────────────────────────────────────────────
        std::string m_prefabName = "StateText";
        engine::Vector3 m_worldOffset{ 0.0f, 2.0f, 0.0f };  // 오브젝트 위 오프셋 (Y축)
        bool m_hideWhenOffscreen = true;
        
        // 체력 텍스트 설정
        bool m_showHpText = true;
        int m_hpFontSize = 48;           // 체력 텍스트 폰트 크기
        int m_stateFontSize = 32;        // 상태 텍스트 폰트 크기
        float m_hpTextYOffset = 30.0f;   // 체력 텍스트가 상태 텍스트 위로 올라가는 픽셀

        // ─────────────────────────────────────────────
        // 캐시된 뷰포트 정보
        // ─────────────────────────────────────────────
        float m_cachedVpW = -1.0f;
        float m_cachedVpH = -1.0f;
        float m_cachedParentRectX = 0.0f;
        float m_cachedParentRectY = 0.0f;

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
        void FindAllControllers();
        void CreateTextForController(BaseControllerScript* controller);
        void UpdateTrackedObject(TrackedObject& tracked);
        void SetTextVisible(TrackedObject& tracked, bool visible);
        void CleanupDestroyedObjects();
        
        // 월드 좌표 → 스크린 좌표 변환
        bool WorldToScreen(const engine::Vector3& worldPos, engine::Vector2& screenPos);
    };
}
