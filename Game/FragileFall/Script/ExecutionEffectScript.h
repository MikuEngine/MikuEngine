#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // ExecutionEffectScript - 처형 이펙트 (독립 라이프사이클)
    // 
    // 기능:
    //   - 생성 시 스케일 애니메이션 시작
    //   - 애니메이션 완료 후 자동 소멸
    // 
    // 사용법:
    //   - 프리팹에 이 스크립트 추가
    //   - ExecutionIndicatorManager에서 인스턴시에이트
    // ═══════════════════════════════════════════════════════════════
    class ExecutionEffectScript :
        public engine::Script<ExecutionEffectScript>
    {
        REGISTER_SCRIPT(ExecutionEffectScript, Script)

    private:
        // ─────────────────────────────────────────────
        // 이펙트 설정
        // ─────────────────────────────────────────────
        float m_duration = 0.2f;               // 이펙트 지속 시간 (초)
        float m_scaleMultiplier = 1.5f;        // 최종 스케일 배율

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_timer = 0.0f;
        engine::Vector3 m_initialScale;
        bool m_initialized = false;

    public:
        void Start() override;
        void Update() override;

        // 외부에서 설정 가능
        void SetDuration(float duration) { m_duration = duration; }
        void SetScaleMultiplier(float multiplier) { m_scaleMultiplier = multiplier; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
