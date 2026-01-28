#include "GamePCH.h"
#include "BossShieldEffect.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Object/Component/Transform.h>

namespace game
{
    void BossShieldEffect::Start()
    {
        m_shieldMesh = GetGameObject()->GetComponent<engine::StaticMeshRenderer>();
        m_shieldMesh->SetObstacleAlpha(true, 0.5f);
    }

    void BossShieldEffect::Update()
    {
        if (!m_shieldMesh) return;

        float deltaTime = engine::Time::DeltaTime();
        m_pulseTimer += deltaTime * m_pulseSpeed;

        // 쉴드 이펙트 애니메이션 (펄스 효과)
        // 스케일을 펄스로 변경 (1.0 ~ 1.1 사이)
        float pulseScale = 0.7f + 0.3f * (0.5f + 0.5f * std::sinf(m_pulseTimer));

        auto* transform = GetGameObject()->GetTransform();
        if (transform)
        {
            engine::Vector3 currentScale = transform->GetLocalScale();
            transform->SetLocalScale(engine::Vector3(pulseScale, pulseScale, pulseScale));
        }

        // TODO: 알파 펄스, 회전 등 추가 효과
    }
}
