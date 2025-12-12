#include "pch.h"
#include "Scene.h"

namespace engine
{
    void Scene::Enter()
    {

    }

    void Scene::Exit()
    {
        m_gameObjects.clear();
    }

    GameObject* Scene::CreateGameObject(std::string_view name)
    {
        m_gameObjects.push_back(std::make_unique<GameObject>());

        return m_gameObjects.back().get();
    }
}