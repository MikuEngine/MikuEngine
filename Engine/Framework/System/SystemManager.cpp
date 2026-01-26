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
#include "Framework/System/UIEventSystem.h"
#include "Framework/System/SoundSystem.h"
#include "Framework/System/ParticleSystem.h"
#include "Framework/System/PathfindingSystem.h"
#include "Framework/System/EnvironmentSystem.h"
#include "Framework/System/PostProcessingSystem.h"

namespace engine
{
    SystemManager::SystemManager()
        : m_scriptSystem{ std::make_unique<ScriptSystem>() },
        m_transformSystem{ std::make_unique<TransformSystem>() },
        m_renderSystem{ std::make_unique<RenderSystem>() },
        m_cameraSystem{ std::make_unique<CameraSystem>() },
        m_animatorSystem{ std::make_unique<AnimatorSystem>() },
        m_lightSystem{ std::make_unique<LightSystem>() },
        m_uiEventSystem{ std::make_unique<UIEventSystem>() },
        m_particleSystem{ std::make_unique<ParticleSystem>() },
        m_physicsSystem{ std::make_unique<PhysicsSystem>() },
        m_collisionSystem{ std::make_unique<CollisionSystem>() },
        m_pathfindingSystem{ std::make_unique<PathfindingSystem>() },
        m_environmentSystem{ std::make_unique<EnvironmentSystem>() },
        m_postProcessingSystem{ std::make_unique<PostProcessingSystem>() }
    {
        // SoundSystem만 Singleton으로 남아있음
    }

    SystemManager::~SystemManager() = default;

    void SystemManager::Shutdown()
    {
        // 물리 시스템 먼저 종료 (다른 시스템에서 참조할 수 있으므로)
        // 소멸자에서 자동으로 리소스 해제됨
        m_collisionSystem.reset();  // STL 컨테이너만 있으므로 자동 정리
        m_physicsSystem.reset();    // 소멸자에서 PhysX 리소스 해제
        
        // 나머지 시스템 종료
        m_scriptSystem.reset();
        m_transformSystem.reset();
        m_renderSystem.reset();
        m_cameraSystem.reset();
        m_animatorSystem.reset();
        m_lightSystem.reset();
        m_uiEventSystem.reset();
        m_particleSystem.reset();
        m_pathfindingSystem.reset();
        m_environmentSystem.reset();
        m_postProcessingSystem.reset();

        // Singleton
        SoundSystem::Get().Shutdown();
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

    ParticleSystem& SystemManager::GetParticleSystem() const
    {
        return *m_particleSystem.get();
    }

    PathfindingSystem& SystemManager::GetPathfindingSystem() const
    {
        return *m_pathfindingSystem.get();
    }

    EnvironmentSystem& SystemManager::GetEnvironmentSystem() const
    {
        return *m_environmentSystem.get();
    }

    PostProcessingSystem& SystemManager::GetPostProcessingSystem() const
    {
        return *m_postProcessingSystem.get();
    }

    PhysicsSystem& SystemManager::GetPhysicsSystem() const
    {
        return *m_physicsSystem.get();
    }

    CollisionSystem& SystemManager::GetCollisionSystem() const
    {
        return *m_collisionSystem.get();
    }

    UIEventSystem& SystemManager::GetUIEventSystem() const
    {
        return *m_uiEventSystem.get();
    }

    SoundSystem& SystemManager::GetSoundSystem() const
    {
        return SoundSystem::Get();
    }
}