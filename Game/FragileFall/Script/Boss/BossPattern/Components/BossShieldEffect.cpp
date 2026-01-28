#include "GamePCH.h"
#include "BossShieldEffect.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Object/Component/Transform.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossShieldEffect::Awake()
    {
        if (GetGameObject())
        {
            m_shieldMesh = GetGameObject()->GetComponent<engine::StaticMeshRenderer>();
        }
    }

    void BossShieldEffect::Update()
    {
        if (!m_shieldMesh) return;

        float deltaTime = engine::Time::DeltaTime();
        m_pulseTimer += deltaTime * m_pulseSpeed;

        // TODO: 쉴드 이펙트 애니메이션 (펄스, 회전 등)
        // 예: 스케일 펄스, 알파 펄스 등
    }
}
