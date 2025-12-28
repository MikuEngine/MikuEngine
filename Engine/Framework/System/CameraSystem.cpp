#include "pch.h"
#include "CameraSystem.h"

namespace engine
{
    void CameraSystem::Register(Camera* camera)
    {
        System::Register(camera);

        m_mainCamera = camera;

        auto viewport = GraphicsDevice::Get().GetViewport();

        m_mainCamera->SetWidth(viewport.Width);
        m_mainCamera->SetHeight(viewport.Height);
    }

    void CameraSystem::Unregister(Camera* camera)
    {
        System::Unregister(camera);

        m_mainCamera = nullptr;
    }

    void CameraSystem::Update()
    {
        if (m_mainCamera != nullptr)
        {
            m_mainCamera->Update();
        }
    }

    Camera* CameraSystem::GetMainCamera() const
    {
        return m_mainCamera;
    }
}