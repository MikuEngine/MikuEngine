#include "GamePCH.h"
#include "BossPatternManager.h"

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/BossPatternBase.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생성자/소멸자
    // ═══════════════════════════════════════════════════════════════
    BossPatternManager::BossPatternManager()
        : m_boss(nullptr)
        , m_independentPattern(nullptr)
    {
    }

    BossPatternManager::~BossPatternManager()
    {
    }

    // ═══════════════════════════════════════════════════════════════
    // 초기화
    // ═══════════════════════════════════════════════════════════════
    void BossPatternManager::Initialize(BossScript* boss)
    {
        m_boss = boss;
    }

    void BossPatternManager::RegisterPattern(std::unique_ptr<BossPatternBase> pattern)
    {
        if (!pattern) return;

        // 독립 패턴이 아니면 일반 패턴으로 등록
        if (pattern.get() != m_independentPattern)
        {
            m_normalPatterns.push_back(pattern.get());
        }

        m_patterns.push_back(std::move(pattern));
    }

    void BossPatternManager::SetIndependentPattern(BossPatternBase* pattern)
    {
        m_independentPattern = pattern;
    }

    // ═══════════════════════════════════════════════════════════════
    // 패턴 실행
    // ═══════════════════════════════════════════════════════════════
    void BossPatternManager::Update(float deltaTime)
    {
        if (!m_boss) return;

        // 독립 패턴 (기둥 쉴드) - 시간 기반 독립 실행
        if (m_independentPattern)
        {
            m_independentPattern->Update(m_boss, deltaTime);

            // 간격 타이머 체크
            // TODO: BossPatternBase에 intervalTimer 관리 로직 추가 필요
            // 현재는 각 패턴에서 직접 관리하도록 설계
        }

        // 일반 패턴들 - 독립적이지 않은 방식으로 실행 (구현 시 결정)
        // TODO: 일반 패턴 실행 로직 구현
    }

    // ═══════════════════════════════════════════════════════════════
    // 패턴 조회
    // ═══════════════════════════════════════════════════════════════
    BossPatternBase* BossPatternManager::GetPattern(const std::string& patternName) const
    {
        for (const auto& pattern : m_patterns)
        {
            if (pattern && pattern->GetPatternName() == patternName)
            {
                return pattern.get();
            }
        }
        return nullptr;
    }
}
