#pragma once

#include "Common/Utility/Singleton.h"

namespace engine
{
    class ScriptSystem;
    class TransformSystem;
    class RenderSystem;

    class SystemManager :
        public Singleton<SystemManager>
    {
    private:
        std::unique_ptr<ScriptSystem> m_scriptSystem;
        std::unique_ptr<TransformSystem> m_transformSystem;
        std::unique_ptr<RenderSystem> m_renderSystem;

    private:
        SystemManager();
        ~SystemManager();

    public:
        ScriptSystem& Script() const;
        TransformSystem& Transform() const;
        RenderSystem& Render() const;

    private:
        friend class Singleton<SystemManager>;
    };
}
