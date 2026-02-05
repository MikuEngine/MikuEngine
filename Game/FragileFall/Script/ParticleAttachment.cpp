#include "GamePCH.h"
#include "ParticleAttachment.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Particle/ParticleEffect.h"
#include "Framework/Object/Component/Transform.h"

namespace game
{
    void ParticleAttachment::LateUpdate()
    {
        if (m_target)
        {
            GetTransform()->SetWorldMatrix(m_target->GetTransform()->GetWorld());
        }
        else if (!m_isTargetDestroyed)
        {
            m_isTargetDestroyed = true;

            if (auto* effect = GetGameObject()->GetComponent<engine::ParticleEffect>())
            {
                effect->Stop();
                effect->SetAutoDestroy(true);
            }
        }
    }

    void ParticleAttachment::SetTarget(engine::GameObject* target)
    {
        m_target = target;
        if (m_target)
        {
            GetTransform()->SetWorldMatrix(m_target->GetTransform()->GetWorld());
        }
    }

    void ParticleAttachment::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void ParticleAttachment::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}