#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/EnvironmentSettings.h"

namespace engine
{
    class EnvironmentSystem :
        public System<EnvironmentSettings>
    {
    public:
        EnvironmentSettings* GetEnvironmentSettings() const;
    };
}
