#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // ExecutionSlowScript - 처형 시 슬로우 모션 효과
    // 
    // 기능:
    //   - 처형 액션 시 TimeScaler를 통해 슬로우 모션 적용
    //   - 크로스페이드 기반 (진입 → 유지 → 이탈)
    // 
    // 사용법:
    //   - 씬에 별도 오브젝트 생성 후 이 스크립트 추가
    //   - ExecutionIndicatorManager에서 StartSlowMotion() 호출
    // ═══════════════════════════════════════════════════════════════
    class ExecutionSlowScript :
        public engine::Script<ExecutionSlowScript>
    {
        REGISTER_SCRIPT(ExecutionSlowScript, Script)

    private:
        // ─────────────────────────────────────────────
        // 슬로우 효과 설정
        // ─────────────────────────────────────────────
        float m_slowDuration = 5.0f;           // 총 슬로우 지속 시간 (실제 시간)
        float m_targetTimeScale = 0.2f;        // 목표 타임스케일 (0.2 = 5배 느림)
        bool m_useCrossfade = false;           // 크로스페이드 사용 여부 (테스트용 false)
        
        // 크로스페이드 비율 (합계 1.0)
        float m_fadeInRatio = 0.3f;            // 진입 비율 (30%)
        float m_sustainRatio = 0.4f;           // 유지 비율 (40%)
        float m_fadeOutRatio = 0.3f;           // 이탈 비율 (30%)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isActive = false;

    public:
        void Start() override;
        void Update() override;

    public:
        // ExecutionIndicatorManager에서 호출
        void StartSlowMotion();
        void StopSlowMotion();
        
        bool IsActive() const { return m_isActive; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
