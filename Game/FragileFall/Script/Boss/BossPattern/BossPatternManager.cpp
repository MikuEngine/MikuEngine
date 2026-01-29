#include "GamePCH.h"
#include "BossPatternManager.h"

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/BossPatternBase.h"

namespace game
{
    BossPatternManager::BossPatternManager() = default;
    BossPatternManager::~BossPatternManager() = default;

    void BossPatternManager::Initialize(BossScript* boss)
    {
        m_boss = boss;
    }

    void BossPatternManager::RegisterPattern(std::unique_ptr<BossPatternBase> pattern)
    {
        if (!pattern)
        {
            return;
        }

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
        m_independentPattern->Start(m_boss);
    }

    void BossPatternManager::Update(float deltaTime)
    {
        if (m_independentPattern)
        {
            m_independentPattern->Update(m_boss, deltaTime);
        }

        // 일반 패턴들 - 각 패턴이 독립적으로 interval을 체크하여 실행
        for (auto* pattern : m_normalPatterns)
        {
            if (!pattern)
            {
                continue;
            }

            // 패턴이 이미 실행 중이면 Update만 호출
            if (pattern->IsActive())
            {
                pattern->Update(m_boss, deltaTime);
                
                // 패턴이 완료되었으면 종료
                if (pattern->IsFinished())
                {
                    pattern->End(m_boss);
                }
            }
            else
            {
                // 패턴이 실행 중이 아니면 interval 체크
                // 각 패턴은 자신의 intervalTimer를 관리
                // 여기서는 간단히 Update를 호출하고, 패턴 내부에서 interval 체크
                pattern->Update(m_boss, deltaTime);
            }
        }
    }
    
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
