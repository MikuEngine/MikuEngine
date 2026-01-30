#include "EnginePCH.h"
#include "CollisionTypes.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Collider.h"

namespace engine
{
    QueryFilter::QueryFilter(uint32_t mask)
        : layerMask(mask)
    {
    }

    QueryFilter::QueryFilter(GameObject* ignore, uint32_t mask)
        : ignoreObject(ignore), layerMask(mask)
    {
    }

    QueryFilter::QueryFilter(Ptr<GameObject> ignore, uint32_t mask)
        : ignoreObject(ignore), layerMask(mask)
    {
    }
}
