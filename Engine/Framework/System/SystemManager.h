#pragma once

#include "Common/Utility/Singleton.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionSystem.h"

namespace engine
{
    class ScriptSystem;
    class TransformSystem;
    class RenderSystem;
    class CameraSystem;
    class AnimatorSystem;
    class LightSystem;

    class SystemManager :
        public Singleton<SystemManager>
    {
    private:
        std::unique_ptr<ScriptSystem> m_scriptSystem;
        std::unique_ptr<TransformSystem> m_transformSystem;
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<CameraSystem> m_cameraSystem;
        std::unique_ptr<AnimatorSystem> m_animatorSystem;
        std::unique_ptr<LightSystem> m_lightSystem;

        // PhysicsSystem과 CollisionSystem은 Singleton이므로 별도 멤버 불필요

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

        // 물리 시스템 접근 (Singleton 래핑)
        PhysicsSystem& GetPhysicsSystem() const;
        CollisionSystem& GetCollisionSystem() const;

    private:
        friend class Singleton<SystemManager>;
    };
}
