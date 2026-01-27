#pragma once

namespace game
{
    class BossScript;
    class BossPatternBase;

    // ═══════════════════════════════════════════════════════════════
    // BossPatternManager - 패턴 관리자
    // 
    // 목적:
    //   - 보스 패턴 등록 및 실행 관리
    //   - 독립 패턴(기둥 쉴드)과 일반 패턴들을 분리 관리
    // 
    // 동작:
    //   - 독립 패턴: 시간 기반 독립 실행 (N초마다 자동 실행)
    //   - 일반 패턴: 독립적이지 않은 방식으로 실행 (구현 시 결정)
    // ═══════════════════════════════════════════════════════════════
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

        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        void Initialize(BossScript* boss);
        void RegisterPattern(std::unique_ptr<BossPatternBase> pattern);
        void SetIndependentPattern(BossPatternBase* pattern);  // 기둥 쉴드 패턴 설정 (독립 패턴)

        // ─────────────────────────────────────────────
        // 패턴 실행
        // ─────────────────────────────────────────────
        void Update(float deltaTime);
        // - 독립 패턴(기둥 쉴드): 시간 기반 자동 실행
        // - 일반 패턴들: 독립적이지 않은 방식으로 실행 (구현 시 결정)

        // ─────────────────────────────────────────────
        // 패턴 조회
        // ─────────────────────────────────────────────
        BossPatternBase* GetPattern(const std::string& patternName) const;
    };
}
