#pragma once

#include <functional>

#include "Common/Utility/Singleton.h"

namespace engine
{
    class LoadingScreenRenderer :
        public Singleton<LoadingScreenRenderer>
    {
        friend class Singleton<LoadingScreenRenderer>;

    private:
        LoadingScreenRenderer() = default;
        ~LoadingScreenRenderer() = default;

    private:
        std::function<void(float)> m_renderCallback;

    public:
        void SetRenderCallback(std::function<void(float)> fn);
        void Render(float progress);
    };
}
