#pragma once

namespace game
{
    class BossScript;
    class BossPatternBase;

    class BossPatternManager
    {
    private:
        BossScript* m_boss = nullptr;
        std::vector<std::unique_ptr<BossPatternBase>> m_patterns;  // 등록된 패턴들

        // 독립 패턴 (기둥 쉴드) - 시간 기반 독립 실행
        BossPatternBase* m_independentPattern = nullptr;  // Pattern_PillarShield

        // 일반 패턴들 (4가지) - 독립적이지 않음
        std::vector<BossPatternBase*> m_normalPatterns;  // 일반 패턴들

    public:
        BossPatternManager();
        ~BossPatternManager();

        void Initialize(BossScript* boss);
        void RegisterPattern(std::unique_ptr<BossPatternBase> pattern);
        void SetIndependentPattern(BossPatternBase* pattern);  // 기둥 쉴드 패턴 설정 (독립 패턴)

        void Update(float deltaTime);

        BossPatternBase* GetPattern(const std::string& patternName) const;
    };
}
