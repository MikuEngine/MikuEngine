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

    class BossScript :
        public engine::Script<BossScript>
    {
        REGISTER_SCRIPT(BossScript, Script)

    public:
        enum class BossColor
        {
            Red,
            Blue,
            Green,
            Yellow,
            Purple
        };

    private:
        int m_maxHp = 10000;
        int m_currentHp = 10000;

        BossColor m_currentColor = BossColor::Red;

        // shield
        bool m_isShieldActive = false;
        std::vector<engine::Ptr<BossPillar>> m_activePillars;

        // pattern
        std::unique_ptr<BossPatternManager> m_patternManager;

        // 간단한 움직임용 수정 게임오브젝트
        std::vector<engine::Ptr<engine::GameObject>> m_crystalMeshGameObjects;  // 보스 수정 메쉬들

        // player
        engine::Ptr<PlayerControllerScript> m_targetPlayer;

        bool m_canBeCrystallized = true;  // 구체 패턴의 투사체 결정화 가능 여부

    public:
        BossScript();
        ~BossScript();

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void InitializePatterns();
        void InitializeCrystalMeshes();  // 수정 메쉬들 초기화 및 회전 설정

        // pattern
        void UpdatePatternSystem(float deltaTime);
        void OnPatternStarted(const std::string& patternName);
        void OnPatternFinished(const std::string& patternName);

        void TakeDamage(int damage);
        void CheckHealth();
        void OnDeath();

        // shield
        void UpdateShieldStatus();  // 기둥 상태 체크하여 쉴드 활성화/비활성화
        bool IsShieldActive() const { return m_isShieldActive; }
        void OnPillarCreated(engine::Ptr<BossPillar> pillar);
        void OnPillarDestroyed(engine::Ptr<BossPillar> pillar);

        void SetColor(BossColor color);
        BossColor GetCurrentColor() const { return m_currentColor; }
        std::string GetColorName() const;

        int GetCurrentHP() const { return m_currentHp; }
        int GetMaxHP() const { return m_maxHp; }

        engine::Ptr<PlayerControllerScript> GetTargetPlayer() const;

        void OnCrystallized();  // 결정화 상태 진입
        void OnExecutionReflected(engine::Vector3 direction);  // 처형 반사 시 호출

        void UpdateCrystalMovement(float deltaTime);  // 수정 메쉬들 회전 업데이트

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
