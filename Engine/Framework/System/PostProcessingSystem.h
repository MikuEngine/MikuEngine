#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/PostProcessingSettings.h"

namespace engine
{
    class PostProcessingSystem :
        public System<PostProcessingSettings>
    {
    public:
        PostProcessingSettings* GetPostProcessingSettings() const;
    };
}
