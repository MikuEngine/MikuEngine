#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class StaticMeshRenderer;
    class Transform;
}

namespace game
{
    class PlayerControllerScript;
    class BossPatternManager;
    class BossPillar;

    // ═══════════════════════════════════════════════════════════════
    // BossScript - 보스 기본 클래스
    // 
    // 목적:
    //   - 수정(Crystal) 형태 보스의 핵심 로직 및 패턴 시스템 통합
    //   - 제자리에 고정된 정적 보스
    //   - StaticMesh 기반, 애니메이션 없음
    // 
    // 주요 기능:
    //   - 패턴 시스템 관리
    //   - 쉴드 시스템 (기둥 패턴)
    //   - 색상 시스템 (5종류 색상)
    //   - 결정화/처형 시스템 연동
    // ═══════════════════════════════════════════════════════════════
    class BossScript :
        public engine::Script<BossScript>
    {
        REGISTER_SCRIPT(BossScript, Script)

    public:
        // ─────────────────────────────────────────────
        // 색상 시스템
        // ─────────────────────────────────────────────
        enum class BossColor { Red, Blue, Green, Yellow, Purple };

    private:
        // ─────────────────────────────────────────────
        // 보스 스탯
        // ─────────────────────────────────────────────
        int m_maxHp = 10000;
        int m_currentHp = 10000;

        // ─────────────────────────────────────────────
        // 색상 시스템
        // ─────────────────────────────────────────────
        BossColor m_currentColor = BossColor::Red;

        // ─────────────────────────────────────────────
        // 쉴드 시스템 (기둥 패턴)
        // ─────────────────────────────────────────────
        bool m_isShieldActive = false;  // 기둥이 하나라도 살아있으면 true
        std::vector<engine::Ptr<BossPillar>> m_activePillars;  // 활성화된 기둥들

        // ─────────────────────────────────────────────
        // 패턴 시스템
        // ─────────────────────────────────────────────
        std::unique_ptr<BossPatternManager> m_patternManager;

        // ─────────────────────────────────────────────
        // StaticMesh 참조 (회전 등 간단한 움직임용)
        // ─────────────────────────────────────────────
        std::vector<engine::StaticMeshRenderer*> m_crystalMeshes;  // 보스 수정 메쉬들
        engine::Transform* m_mainTransform = nullptr;

        // ─────────────────────────────────────────────
        // 플레이어 참조
        // ─────────────────────────────────────────────
        engine::Ptr<PlayerControllerScript> m_targetPlayer;
        std::string m_targetPlayerObjectName = "Player";

        // ─────────────────────────────────────────────
        // 결정화/처형 시스템 연동
        // ─────────────────────────────────────────────
        bool m_canBeCrystallized = true;  // 구체 패턴의 투사체 결정화 가능 여부

    public:
        BossScript();
        ~BossScript();

    public:
        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Awake() override;
        void Start() override;
        void Update() override;

        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        void CacheComponents();
        void InitializePatterns();
        void InitializeCrystalMeshes();  // 수정 메쉬들 초기화 및 회전 설정

        // ─────────────────────────────────────────────
        // 패턴 시스템
        // ─────────────────────────────────────────────
        void UpdatePatternSystem(float deltaTime);
        void OnPatternStarted(const std::string& patternName);
        void OnPatternFinished(const std::string& patternName);

        // ─────────────────────────────────────────────
        // 체력 관리
        // ─────────────────────────────────────────────
        void TakeDamage(int damage);
        void CheckHealth();
        void OnDeath();

        // ─────────────────────────────────────────────
        // 쉴드 시스템
        // ─────────────────────────────────────────────
        void UpdateShieldStatus();  // 기둥 상태 체크하여 쉴드 활성화/비활성화
        bool IsShieldActive() const { return m_isShieldActive; }
        void OnPillarCreated(engine::Ptr<BossPillar> pillar);
        void OnPillarDestroyed(engine::Ptr<BossPillar> pillar);

        // ─────────────────────────────────────────────
        // 색상 시스템
        // ─────────────────────────────────────────────
        void SetColor(BossColor color);
        BossColor GetCurrentColor() const { return m_currentColor; }
        std::string GetColorName() const;

        // ─────────────────────────────────────────────
        // 체력 조회
        // ─────────────────────────────────────────────
        int GetCurrentHP() const { return m_currentHp; }
        int GetMaxHP() const { return m_maxHp; }

        // ─────────────────────────────────────────────
        // 결정화/처형 시스템
        // ─────────────────────────────────────────────
        void OnCrystallized();  // 결정화 상태 진입
        void OnExecutionReflected(engine::Vector3 direction);  // 처형 반사 시 호출

        // ─────────────────────────────────────────────
        // 간단한 움직임 (회전 등)
        // ─────────────────────────────────────────────
        void UpdateCrystalMovement(float deltaTime);  // 수정 메쉬들 회전 업데이트
    };
}
