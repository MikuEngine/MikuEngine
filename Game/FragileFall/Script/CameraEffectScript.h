#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class Camera;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // CameraEffectScript
    // 
    // 카메라 줌 이펙트 스크립트
    // - 플레이어 처형 액션 시 슬로우 효과와 함께 사용
    // - FOV 조절을 통한 줌인/줌아웃
    // - 플레이어를 화면 중앙에 두는 방향으로 XZ 이동
    // - 원본 카메라 시야 영역을 벗어나지 않도록 클램핑
    // - fadeIn → sustain → fadeOut 크로스페이드
    // ═══════════════════════════════════════════════════════════════

    class CameraEffectScript :
        public engine::Script<CameraEffectScript>
    {
        REGISTER_SCRIPT(CameraEffectScript, Script)

    public:
        // 이펙트 상태
        enum class EffectState
        {
            Idle,       // 대기 (이펙트 없음)
            FadeIn,     // 줌인 진행 중
            Sustain,    // 줌인 유지
            FadeOut     // 줌아웃 진행 중
        };

    private:
        // ─────────────────────────────────────────────
        // 에디터 설정 (직렬화)
        // ─────────────────────────────────────────────
        float m_zoomScale = 1.3f;           // 줌 배율 (1.3 = 1.3배 확대)
        float m_fadeInRatio = 0.15f;        // fadeIn 비율 (0~1)
        float m_fadeOutRatio = 0.15f;       // fadeOut 비율 (0~1)
        // sustain = 1.0 - fadeIn - fadeOut (자동 계산)
        
        float m_configuredBaseFov = 60.0f;  // 기본 FOV (에디터에서 설정, Camera 컴포넌트와 동일하게)
        
        // 카메라 이동 가능 경계 (에디터에서 직접 설정)
        float m_moveBoundsMinX = -22.0f;
        float m_moveBoundsMaxX = 22.0f;
        float m_moveBoundsMinZ = -35.0f;
        float m_moveBoundsMaxZ = -9.0f;
        
        // 이펙트 기본 듀레이션 (에디터에서 설정)
        float m_defaultDuration = 2.0f;

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        EffectState m_state = EffectState::Idle;
        
        // 초기 카메라 상태 (Start에서 한 번만 저장, 절대 변경 안 함)
        engine::Vector3 m_initialPosition;
        float m_initialFov = 60.0f;
        bool m_isInitialized = false;
        
        // 원본 카메라 시야 영역 (바닥 Y=0 투영, 4개 코너)
        engine::Vector3 m_baseViewCorners[4];
        
        // 목표 상태
        engine::Vector3 m_targetPosition;
        float m_targetFov = 46.0f;          // baseFov / zoomScale
        
        // 이펙트 시작 시점의 상태 (중첩 처리용)
        engine::Vector3 m_effectStartPosition;
        float m_effectStartFov = 60.0f;     // 이펙트 시작 시점의 FOV
        
        // 현재 FOV 추적 (재호출 시 현재값 유지를 위해)
        float m_currentFov = 60.0f;
        
        // 타이밍
        float m_totalDuration = 0.0f;       // 전체 이펙트 지속시간
        float m_fadeInDuration = 0.0f;      // fadeIn 구간 시간
        float m_sustainDuration = 0.0f;     // sustain 구간 시간
        float m_fadeOutDuration = 0.0f;     // fadeOut 구간 시간
        float m_elapsedTime = 0.0f;         // 현재 경과 시간
        float m_stateElapsedTime = 0.0f;    // 현재 상태 내 경과 시간
        
        // 컴포넌트 캐시
        engine::Camera* m_camera = nullptr;
        engine::GameObject* m_player = nullptr;

    public:
        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Awake() override;
        void Start() override;
        void Update() override;

        // ─────────────────────────────────────────────
        // 외부 호출 인터페이스
        // ─────────────────────────────────────────────
        
        /// @brief 줌 이펙트 시작
        /// @param duration 전체 이펙트 지속시간 (타임스케일 적용됨)
        void StartZoomEffect(float duration);
        
        /// @brief 이펙트 즉시 중단 및 기본 상태로 복귀
        void StopEffect();
        
        /// @brief 현재 이펙트 진행 중인지 확인
        bool IsEffectActive() const { return m_state != EffectState::Idle; }
        
        /// @brief 현재 상태 반환
        EffectState GetState() const { return m_state; }
        
        /// @brief 기본 듀레이션 반환 (외부에서 호출 시 사용)
        float GetDefaultDuration() const { return m_defaultDuration; }

    public:
        // ─────────────────────────────────────────────
        // 직렬화 / 에디터
        // ─────────────────────────────────────────────
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        // ─────────────────────────────────────────────
        // 내부 로직
        // ─────────────────────────────────────────────
        
        /// @brief 원본 카메라의 바닥 투영 영역 계산
        void CalculateBaseViewCorners();
        
        /// @brief 플레이어를 화면 중앙에 두는 목표 위치 계산
        /// @return 클램핑된 목표 위치
        engine::Vector3 CalculateTargetPosition();
        
        /// @brief 주어진 위치에서 줌인된 카메라의 시야가 원본 영역 안에 있는지 확인
        /// @param position 테스트할 카메라 위치
        /// @return 원본 영역 안에 있으면 true
        bool IsPositionWithinBounds(const engine::Vector3& position);
        
        /// @brief 목표 위치를 원본 시야 영역 안으로 클램핑
        /// @param idealPosition 이상적인 목표 위치
        /// @return 클램핑된 위치
        engine::Vector3 ClampPositionToBounds(const engine::Vector3& idealPosition);
        
        /// @brief 특정 카메라 설정으로 바닥에 투영되는 4개 코너 계산
        /// @param position 카메라 위치
        /// @param fov 카메라 FOV
        /// @param outCorners 출력: 4개 코너 좌표
        void CalculateViewCornersAt(const engine::Vector3& position, float fov, engine::Vector3 outCorners[4]);
        
        /// @brief 점이 사각형 영역 안에 있는지 확인 (XZ 평면)
        /// @param point 테스트할 점
        /// @param corners 사각형의 4개 코너 (순서대로)
        /// @return 영역 안에 있으면 true
        bool IsPointInQuad(const engine::Vector2& point, const engine::Vector3 corners[4]);
        
        /// @brief 현재 진행률에 따른 보간값 계산 (0~1)
        float CalculateBlendFactor() const;
        
        /// @brief 카메라 위치와 FOV 업데이트
        void UpdateCameraTransform(float blendFactor);
        
        /// @brief 상태 전이 처리
        void TransitionToNextState();
    };
}
