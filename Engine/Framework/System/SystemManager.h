#pragma once

#include "Common/Utility/Singleton.h"

namespace engine
{
    class ScriptSystem;
    class TransformSystem;
    class RenderSystem;
    class CameraSystem;

    class SystemManager :
        public Singleton<SystemManager>
    {
    private:
        std::unique_ptr<ScriptSystem> m_scriptSystem;
        std::unique_ptr<TransformSystem> m_transformSystem;
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<CameraSystem> m_cameraSystem;

    private:
        SystemManager();
        ~SystemManager();

    public:
        void Shutdown();

    public:
        ScriptSystem& GetScriptSystem() const;
        TransformSystem& GetTransformSystem() const;
        RenderSystem& GetRenderSystem() const;
        CameraSystem& GetCameraSystem() const;

    private:
        friend class Singleton<SystemManager>;
    };
}
