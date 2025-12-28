#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Camera.h"

namespace engine
{
    class CameraSystem :
        public System<Camera>
    {
    private:
        Camera* m_mainCamera = nullptr;

    public:
    public:
        void Register(Camera* camera) override;
        void Unregister(Camera* camera) override;

        void Update();

        Camera* GetMainCamera() const;
    };
}