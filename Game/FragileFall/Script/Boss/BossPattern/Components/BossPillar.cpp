#include "GamePCH.h"
#include "BossPillar.h"
#include "../../BossScript.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossPillar::Awake()
    {
    }

    void BossPillar::Start()
    {
    }

    void BossPillar::Update()
    {
        if (m_hp <= 0 && !m_isDestroyed)
        {
            OnDestroyed();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 데미지 처리
    // ═══════════════════════════════════════════════════════════════
    void BossPillar::TakeDamage(int damage)
    {
        if (m_isDestroyed) return;

        m_hp -= damage;
        if (m_hp < 0)
        {
            m_hp = 0;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 파괴 처리
    // ═══════════════════════════════════════════════════════════════
    void BossPillar::OnDestroyed()
    {
        if (m_isDestroyed) return;

        m_isDestroyed = true;

        // 보스에 기둥 파괴 알림
        if (m_boss)
        {
            m_boss->OnPillarDestroyed(engine::Ptr<BossPillar>(this));
        }

        // TODO: 파괴 이펙트, 사운드 등
        // GameObject 파괴는 보스에서 관리하거나 여기서 처리
    }
}
