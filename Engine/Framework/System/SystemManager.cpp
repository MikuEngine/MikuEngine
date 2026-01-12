#include "EnginePCH.h"
#include "SystemManager.h"

#include "Framework/System/ScriptSystem.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/System/AnimatorSystem.h"
#include "Framework/System/LightSystem.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionSystem.h"

namespace engine
{
    SystemManager::SystemManager()
        : m_scriptSystem{ std::make_unique<ScriptSystem>() },
        m_transformSystem{ std::make_unique<TransformSystem>() },
        m_renderSystem{ std::make_unique<RenderSystem>() },
        m_cameraSystem{ std::make_unique<CameraSystem>() },
        m_animatorSystem{ std::make_unique<AnimatorSystem>() },
        m_lightSystem{ std::make_unique<LightSystem>() }
    {
        // PhysicsSystem과 CollisionSystem은 Singleton으로 자동 관리됨
    }

    SystemManager::~SystemManager() = default;

    void SystemManager::Shutdown()
    {
        m_scriptSystem.reset();
        m_transformSystem.reset();
        m_renderSystem.reset();
        m_cameraSystem.reset();
        m_animatorSystem.reset();
        m_lightSystem.reset();
        
        // 물리 시스템 종료 (Singleton이므로 명시적 호출)
        PhysicsSystem::Get().Shutdown();
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

    AnimatorSystem& SystemManager::GetAnimatorSystem() const
    {
        return *m_animatorSystem.get();
    }

    LightSystem& SystemManager::GetLightSystem() const
    {
        return *m_lightSystem.get();
    }

    PhysicsSystem& SystemManager::GetPhysicsSystem() const
    {
        return PhysicsSystem::Get();
    }

    CollisionSystem& SystemManager::GetCollisionSystem() const
    {
        return CollisionSystem::Get();
    }
}