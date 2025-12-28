#include "pch.h"
#include "Scene.h"

#include <functional>
#include <fstream>

#include "Common/Utility/JsonHelper.h"
#include "Framework/Object/Component/Camera.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"

namespace engine
{
    using json = nlohmann::ordered_json;

    Scene::Scene(const std::string& name, std::function<void()>&& onEnter)
        : m_name{ name }, m_onEnter{ std::move(onEnter) }
    {

    }

    void Scene::Enter()
    {
        LOG_PRINT("{} scene enter", m_name);

        auto gameObject = CreateGameObject("MainCamera");
        gameObject->AddComponent<Camera>();
        
        m_onEnter();
    }

    void Scene::Exit()
    {
        LOG_PRINT("{} scene exit", m_name);
        m_gameObjects.clear();
    }

    void Scene::Save(const std::string& path)
    {
        json root;
        root["Name"] = m_name;
        root["NumGameObjects"] = m_gameObjects.size();
        root["GameObjects"] = json::array();

        std::vector<GameObject*> sortedList;
        sortedList.reserve(m_gameObjects.size());

        std::function<void(Transform*)> traverse = [&](Transform* tr)
            {
                sortedList.push_back(tr->GetGameObject());
                for (auto child : tr->GetChildren())
                {
                    traverse(child);
                }
            };

        for (const auto& go : m_gameObjects)
        {
            if (go->GetTransform()->GetParent() == nullptr)
            {
                traverse(go->GetTransform());
            }
        }

        std::unordered_map<GameObject*, int> ptrToId;
        int currentId = 0;
        for (auto go : sortedList)
        {
            ptrToId[go] = currentId++;
        }

        for (auto go : sortedList)
        {
            json goJson;
            goJson["ID"] = ptrToId[go];

            Transform* parent = go->GetTransform()->GetParent();
            if (parent != nullptr)
            {
                if (auto iter = ptrToId.find(parent->GetGameObject()); iter != ptrToId.end())
                {
                    goJson["ParentID"] = iter->second;
                }
            }

            go->Save(goJson);
            root["GameObjects"].push_back(goJson);
        }

        std::filesystem::path filePath{ path };

        if (filePath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(filePath.parent_path(), ec);
        }

        std::ofstream o(path);
        o << std::setw(4) << root << std::endl;
    }

    void Scene::Load(const std::string& path)
    {
        std::ifstream i(path);
        if (!i.is_open())
        {
            return;
        }

        json root;
        i >> root;

        m_gameObjects.clear();

        JsonGet(root, "Name", m_name);
        size_t numGameObjects;
        JsonGet(root, "NumGameObjects", numGameObjects);


        std::vector<GameObject*> idToPtr(numGameObjects + 1);
        std::vector<std::pair<int, int>> parentLinks;

        JsonArrayForEach(root, "GameObjects", [&](const json& goJson)
            {
                std::string name = goJson.value("Name", "GameObject");
                GameObject* go = CreateGameObject(name);

                int id = goJson.value("ID", -1);
                if (id >= 0 && id < idToPtr.size())
                {
                    idToPtr[id] = go;
                }

                go->Load(goJson);

                int parentId = -1;
                JsonGet(goJson, "ParentID", parentId);
                if (parentId != -1)
                {
                    parentLinks.push_back({ id, parentId });
                }
            });

        for (const auto& link : parentLinks)
        {
            int childId = link.first;
            int parentId = link.second;

            if (childId < idToPtr.size() && parentId < idToPtr.size())
            {
                GameObject* c = idToPtr[childId];
                GameObject* p = idToPtr[parentId];

                if (c != nullptr && p != nullptr)
                {
                    c->GetTransform()->SetParent(p->GetTransform());
                }
            }
        }
    }

    GameObject* Scene::CreateGameObject(const std::string& name)
    {
        m_gameObjects.push_back(std::make_unique<GameObject>());

        return m_gameObjects.back().get();
    }

    Camera* Scene::GetMainCamera() const
    {
        return SystemManager::Get().GetCameraSystem().GetMainCamera();
    }

    const std::vector<std::unique_ptr<GameObject>>& Scene::GetGameObjects() const
    {
        return m_gameObjects;
    }
}