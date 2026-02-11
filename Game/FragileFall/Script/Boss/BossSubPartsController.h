#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class BossSubPartsController :
        public engine::Script<BossSubPartsController>
    {
        REGISTER_SCRIPT(BossSubPartsController, Script)

    private:
        float m_accTime = 0.0f; // 누적 시간 변수
        std::vector<engine::Transform*> m_rotatingSubParts;
        std::vector<engine::Transform*> m_floatingSubParts;
        std::vector<engine::Transform*> m_nestSubParts;

        int m_orbitPartsCount = 20;
        float m_orbitSpeed = 60.0f;
        float m_orbitRadius = 5.0f;
        float m_bobbingSpeed = 1.5f;
        float m_bobbingAmount = 3.5f;
        
        // 타원 궤도 관련 변수들
        float m_ellipseRatioX = 2.0f;         // X축 타원 비율 (1.0이면 원형)
        float m_ellipseRatioZ = 1.0f;         // Z축 타원 비율
        float m_orbitDistortionAmount = 0.3f; // 궤도 왜곡 정도 (0~1)
        float m_orbitDistortionSpeed = 0.8f;  // 궤도 왜곡 변화 속도
        float m_orbitNoiseScale = 2.0f;       // 궤도 노이즈 스케일
        float m_orbitWaveCount = 3.0f;        // 궤도에 적용할 파동 개수

        // Floating Parts 관련 변수들
        float m_floatingSpeed = 2.0f;        // 기본 떠다니는 속도
        float m_floatingAmplitude = 2.0f;    // 위아래 움직임 폭
        float m_floatingSpeedVariation = 0.5f; // 각 파츠별 속도 변화량
        float m_floatingPhaseVariation = 1.0f; // 각 파츠별 위상 변화량

        // Nest Parts 관련 변수들
        float m_nestPunchSpeed = 3.0f;        // 개별 파츠 펀치 속도
        float m_nestPunchAmount = 0.8f;       // 펀치 움직임 폭
        float m_nestPunchVariation = 2.0f;    // 각 파츠별 펀치 타이밍 차이
        float m_nestGroupSpeed = 0.5f;        // 전체 그룹 수축/확장 속도
        float m_nestGroupAmount = 0.3f;       // 중심으로 모이는 정도 (0~1)
        float m_nestGroupCycle = 8.0f;        // 전체 그룹 사이클 주기

        // 전체 떨림 효과 관련 변수들
        float m_shakeIntensity = 0.0f;        // 떨림 강도 (0이면 떨림 없음)
        float m_shakeSpeed = 15.0f;           // 떨림 속도
        float m_baseShakeAmount = 0.05f;      // 기본 미세 떨림량
        float m_hitShakeAmount = 0.5f;        // 피격시 떨림량
        float m_hitShakeDuration = 1.0f;      // 피격 떨림 지속시간
        float m_hitShakeDecay = 3.0f;         // 피격 떨림 감쇠 속도
        
        // 피격 떨림 상태
        float m_currentHitShake = 0.0f;       // 현재 피격 떨림 강도

        // Intro Assemble
        bool m_introActive = false;
        bool m_introAssembled = true;
        float m_introElapsed = 0.0f;
        float m_introAssembleDuration = 1.2f;
        float m_introSpawnRadius = 22.0f;
        float m_introSpawnBaseHeight = 2.5f;
        float m_introSpawnHeightJitter = 5.0f;
        float m_introFrontSpreadDegree = 65.0f;
        int m_introDirectionMode = 0;  // 0: front random, 1: sphere, 2: eight directions
        float m_introStaggerPerPart = 0.02f;
        float m_introStaggerJitter = 0.04f;
        bool m_introStaggerRandomOrder = true;

        std::vector<engine::Transform*> m_introParts;
        std::vector<engine::Vector3> m_introStartLocalPositions;
        std::vector<engine::Vector3> m_introStartLocalRotations;
        std::vector<engine::Vector3> m_introTargetLocalPositions;
        std::vector<engine::Vector3> m_introTargetRotations;
        std::vector<float> m_introPartStartTimes;
        std::vector<engine::Vector3> m_floatingInitialLocalPositions;
        std::vector<engine::Vector3> m_nestInitialLocalPositions;

    private:
        // 떨림 효과 헬퍼 함수
        engine::Vector3 ApplyShakeEffect(const engine::Vector3& originalPos, int partIndex);
        engine::Vector3 ComputeRotatingTargetLocalPosition(size_t partIndex, size_t totalCount) const;
        engine::Vector3 ComputeFloatingTargetLocalPosition(size_t partIndex) const;
        engine::Vector3 ComputeNestTargetLocalPosition(size_t partIndex) const;
        engine::Vector3 ComputeIntroDynamicTargetLocalPosition(size_t introPartIndex) const;
        engine::Vector3 ComputeRotatingTargetLocalRotation(size_t partIndex, size_t totalCount) const;
        engine::Vector3 ComputeIntroDynamicTargetLocalRotation(size_t introPartIndex) const;
        engine::Vector3 GenerateIntroSpawnOffset(size_t partIndex, size_t totalCount) const;
        void UpdateIntroAssembly(float deltaTime);

    public:
        //void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        
        // 피격시 떨림 효과 트리거
        void TriggerHitShake(float intensity = 1.0f);
        void PrepareIntroAssembly();
        bool IsIntroAssemblyComplete() const { return m_introAssembled; }
        void SetIntroAssembleDuration(float duration) { m_introAssembleDuration = (duration < 0.01f) ? 0.01f : duration; }
    };
}