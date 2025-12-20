#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/StaticMeshRenderer.h"

namespace engine
{
    class RenderSystem :
        public System<StaticMeshRenderer>
    {
    public:
        void Render();
    };
}
