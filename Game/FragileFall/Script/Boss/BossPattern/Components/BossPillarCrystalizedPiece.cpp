#include "GamePCH.h"
#include "BossPillarCrystalizedPiece.h"

namespace game
{
    void BossPillarCrystalizedPiece::Start()
    {
        auto scale = GetTransform()->GetLocalScale();
        m_startYScale = scale.y;
    }

    void BossPillarCrystalizedPiece::Update()
    {
        auto scale = GetTransform()->GetLocalScale();

        m_progress += engine::Time::DeltaTime();

        if (m_progress >= 1.0f)
        {
            m_progress = 1.0f;
        }

        scale.y = std::lerp(m_startYScale, m_targetYScale, m_progress);

        GetTransform()->SetLocalScale(scale);
    }

    void BossPillarCrystalizedPiece::OnGui()
    {
    }
    
    void BossPillarCrystalizedPiece::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void BossPillarCrystalizedPiece::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}