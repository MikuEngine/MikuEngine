#pragma once

#include "Common/Utility/Singleton.h"
#include "Framework/System/SoundSystem.h"

namespace engine
{
    class ScriptSystem;
    class TransformSystem;
    class RenderSystem;
    class CameraSystem;
    class AnimatorSystem;
    class LightSystem;
    class UIEventSystem;
    class ParticleSystem;
    class PhysicsSystem;
    class CollisionSystem;
    class PathfindingSystem;
    class EnvironmentSystem;
    class PostProcessingSystem;

    class SystemManager :
        public Singleton<SystemManager>
    {
    private:
        // 시스템 (unique_ptr로 관리)
        std::unique_ptr<ScriptSystem> m_scriptSystem;
        std::unique_ptr<TransformSystem> m_transformSystem;
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<CameraSystem> m_cameraSystem;
        std::unique_ptr<AnimatorSystem> m_animatorSystem;
        std::unique_ptr<LightSystem> m_lightSystem;
        std::unique_ptr<UIEventSystem> m_uiEventSystem;
        std::unique_ptr<ParticleSystem> m_particleSystem;
        std::unique_ptr<PathfindingSystem> m_pathfindingSystem;
        std::unique_ptr<EnvironmentSystem> m_environmentSystem;
        std::unique_ptr<PostProcessingSystem> m_postProcessingSystem;
        
        // 물리 시스템 (unique_ptr로 관리)
        std::unique_ptr<PhysicsSystem> m_physicsSystem;
        std::unique_ptr<CollisionSystem> m_collisionSystem;

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
        AnimatorSystem& GetAnimatorSystem() const;
        LightSystem& GetLightSystem() const;
        ParticleSystem& GetParticleSystem() const;
        PathfindingSystem& GetPathfindingSystem() const;
        EnvironmentSystem& GetEnvironmentSystem() const;
        PostProcessingSystem& GetPostProcessingSystem() const;

        // 물리 시스템 접근
        PhysicsSystem& GetPhysicsSystem() const;
        CollisionSystem& GetCollisionSystem() const;

        // UISystem
        UIEventSystem& GetUIEventSystem() const;

        SoundSystem& GetSoundSystem() const;

    private:
        friend class Singleton<SystemManager>;
    };
}
