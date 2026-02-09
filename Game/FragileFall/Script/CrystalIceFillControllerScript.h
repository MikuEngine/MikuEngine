#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class StaticMeshRenderer;
}

namespace game
{
    // IceFillParams - 셰이더 cbuffer IceFill(b13)과 레이아웃 일치 (16바이트)
    struct IceFillParams
    {
        int   enable = 0;     // 0 = 비활성, 0이 아니면 면 단위 얼음 채우기
        float amount = 0.0f; // 0~1 채움 진행도 (아래 면부터)
        float _pad[2] = {};  // HLSL float2 __pad_iceFill
    };

    // ═══════════════════════════════════════════════════════════════
    // CrystalIceFillControllerScript - 크리스탈 얼음 채우기/줄이기 셰이더 제어
    // - Fragile 진입 시: amount 0 → 1 채우기
    // - Fragile 회복(부활) 시: StartDrain() 호출 시 amount 1 → 0 역방향 연출
    // ═══════════════════════════════════════════════════════════════
    class CrystalIceFillControllerScript :
        public engine::Script<CrystalIceFillControllerScript>
    {
        REGISTER_SCRIPT(CrystalIceFillControllerScript, Script)

    public:
        void Start() override;
        void Update() override;

    public:
        const IceFillParams& GetParams() const { return m_params; }
        IceFillParams& GetParams() { return m_params; }
        void SetParams(const IceFillParams& p) { m_params = p; }

        void SetDuration(float seconds) { m_duration = seconds; }
        float GetDuration() const { return m_duration; }
        void SetStepCount(int count) { m_stepCount = (count > 0) ? count : 1; }
        int GetStepCount() const { return m_stepCount; }

        /// 외부에서 직접 fill amount 설정 (0~1). Fragile 타이머와 동기화용.
        void SetFillAmount(float amount);

        /// 자동 채우기 비활성화 (외부에서 수동 제어 시 사용)
        void SetManualControl(bool manual) { m_manualControl = manual; }
        bool IsManualControl() const { return m_manualControl; }

        /// 부활 시 역방향(1→0) 연출 시작. 드레인에 걸리는 시간(초).
        void StartDrain();
        void SetDrainDuration(float seconds) { m_drainDuration = (seconds >= 0.0f) ? seconds : m_duration; }
        float GetDrainDuration() const { return m_drainDuration; }
        bool IsDraining() const { return m_isDraining; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        IceFillParams m_params;
        float m_duration = 1.0f;       // 채워지는 데 걸리는 시간(초)
        float m_elapsed = 0.0f;        // 채우기 경과 시간
        int m_stepCount = 2;            // 몇 단계로 올릴지 (step 식으로 딱딱)
        engine::StaticMeshRenderer* m_renderer = nullptr;

        // 수동 제어 모드 (자동 채우기 비활성화)
        bool m_manualControl = false;

        // 드레인(회복 시 역방향) 연출
        bool m_isDraining = false;
        float m_drainDuration = 5.0f;  // 줄어드는 데 걸리는 시간(초)
        float m_drainElapsed = 0.0f;   // 드레인 경과 시간
    };
}
