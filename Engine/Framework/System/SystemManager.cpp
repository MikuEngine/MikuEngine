#include "pch.h"
#include "SystemManager.h"

#include "Framework/System/ScriptSystem.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/System/CameraSystem.h"

namespace engine
{
    SystemManager::SystemManager()
        : m_scriptSystem{ std::make_unique<ScriptSystem>() },
        m_transformSystem{ std::make_unique<TransformSystem>() },
        m_renderSystem{ std::make_unique<RenderSystem>() },
        m_cameraSystem{ std::make_unique<CameraSystem>() }
    {
    }

    SystemManager::~SystemManager() = default;

    void SystemManager::Shutdown()
    {
        m_scriptSystem.reset();
        m_transformSystem.reset();
        m_renderSystem.reset();
        m_cameraSystem.reset();
    }

    ScriptSystem& SystemManager::GetScriptSystem() const
    {
        return *m_scriptSystem.get();
    }

    TransformSystem& SystemManager::GetTransformSystem() const
    {
        return *m_transformSystem.get();
    }

    RenderSystem& SystemManager::GetRenderSystem() const
    {
        return *m_renderSystem.get();
    }

    CameraSystem& SystemManager::GetCameraSystem() const
    {
        return *m_cameraSystem.get();
    }
}