#include "EnginePCH.h"
#include "Scene.h"

#include <functional>
#include <fstream>

#include "Common/Utility/JsonHelper.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Camera.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"

namespace engine
{
    GameObject* Scene::CreateGameObject(const std::string& name)
    {
        m_incubator.push_back(std::make_unique<GameObject>());

        GameObject* ptr = m_incubator.back().get();
        ptr->m_name = name;
        ptr->m_sceneIndex = static_cast<int32_t>(m_gameObjects.size() - 1);

        RegisterPendingAdd(ptr);

        return ptr;
    }

    Camera* Scene::GetMainCamera() const
    {
        return SystemManager::Get().GetCameraSystem().GetMainCamera();
    }

    const std::vector<std::unique_ptr<GameObject>>& Scene::GetGameObjects() const
    {
        return m_gameObjects;
    }

    const std::string& Scene::GetName() const
    {
        return m_name;
    }

    void Scene::SetName(std::string_view name)
    {
        m_name = name;
    }

    GameObject* Scene::FindGameObject(const std::string& name)
    {
        for (const auto& gameObject : m_gameObjects)
        {
            if (gameObject->m_name == name)
            {
                return gameObject.get();
            }
        }

        return nullptr;
    }

    void Scene::ResetToDefaultScene()
    {
        m_gameObjects.clear();

        m_gameObjectKillList.clear();
        m_componentKillList.clear();
        m_morgue.clear();

        m_incubator.clear();
        m_gameObjectAddList.clear();
        m_componentAddList.clear();
        m_componentAddProcList.clear();

        auto gameObject = CreateGameObject("MainCamera");
        gameObject->AddComponent<Camera>();
    }

    void Scene::Clear()
    {
        m_gameObjects.clear();

        m_gameObjectKillList.clear();
        m_componentKillList.clear();
        m_morgue.clear();

        m_incubator.clear();
        m_gameObjectAddList.clear();
        m_componentAddList.clear();
        m_componentAddProcList.clear();
    }

    void Scene::RegisterPendingAdd(GameObject* gameObject)
    {
        m_gameObjectAddList.push_back(gameObject);
    }

    void Scene::RegisterPendingAdd(Component* component)
    {
        m_componentAddList.push_back(component);
    }

    void Scene::ProcessPendingAdds(bool isPlaying)
    {
        int safeGuard = 0;
        constexpr int MAX_ITERATIONS = 10;

        while (!m_componentAddList.empty() || !m_incubator.empty())
        {
            if (safeGuard++ > MAX_ITERATIONS)
            {
                LOG_ERROR("Scene::ProcessPendingAdds - 재귀 생성 한계 초과 (Infinite Loop Check)");
                break;
            }

            if (!m_incubator.empty())
            {
                size_t neededCapacity = m_gameObjects.size() + m_incubator.size();
                if (m_gameObjects.capacity() < neededCapacity)
                {
                    m_gameObjects.reserve(neededCapacity * 2);
                }

                for (auto& gameObject : m_incubator)
                {
                    m_gameObjects.push_back(std::move(gameObject));
                    m_gameObjects.back()->m_sceneIndex = static_cast<std::int32_t>(m_gameObjects.size() - 1);
                    m_gameObjects.back()->Awake();
                }
                m_incubator.clear();
            }

            if (!m_componentAddList.empty())
            {
                std::swap(m_componentAddList, m_componentAddProcList);

                for (auto component : m_componentAddProcList)
                {
                    if (component->GetGameObject()->IsPendingKill())
                    {
                        continue;
                    }

                    component->Initialize();
                }

                if (isPlaying)
                {
                    for (auto component : m_componentAddProcList)
                    {
                        if (component->GetGameObject()->IsPendingKill())
                        {
                            continue;
                        }

                        component->Awake();
                        component->MarkAsAwoken();
                    }

                    for (auto component : m_componentAddProcList)
                    {
                        if (component->GetGameObject()->IsPendingKill())
                        {
                            continue;
                        }

                        if (component->IsActive())
                        {
                            component->OnEnable();
                        }
                    }
                }

                m_componentAddProcList.clear();
            }
        }

        m_gameObjectAddList.clear();
    }

    void Scene::RegisterPendingKill(GameObject* gameObject)
    {
        m_gameObjectKillList.push_back(gameObject);
    }

    void Scene::RegisterPendingKill(Component* component)
    {
        m_componentKillList.push_back(component);
    }

    void Scene::ProcessPendingKills()
    {
        // 컴포넌트 먼저 삭제
        for (auto component : m_componentKillList)
        {
            if (component->IsActive())
            {
                component->SetActive(false);
            }

            component->OnDestroy();
            component->GetGameObject()->RemoveComponentFast(component);
        }

        m_componentKillList.clear();

        for (auto gameObject : m_gameObjectKillList)
        {
            std::int32_t index = gameObject->m_sceneIndex;
            std::int32_t lastIndex = static_cast<std::int32_t>(m_gameObjects.size() - 1);

            if (index < 0 || index >= m_gameObjects.size() || m_gameObjects[index].get() != gameObject)
            {
                continue;
            }

            if (gameObject->IsActive())
            {
                gameObject->SetActive(false);
            }

            gameObject->BroadcastOnDestroy();

            m_morgue.push_back(std::move(m_gameObjects[index]));

            if (index != lastIndex)
            {
                m_gameObjects[index] = std::move(m_gameObjects[lastIndex]);

                m_gameObjects[index]->m_sceneIndex = index;
            }

            m_gameObjects.pop_back();
        }

        m_gameObjectKillList.clear();
        m_morgue.clear();
    }

    void Scene::RemoveGameObjectEditor(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return;
        }
        
        std::erase_if(m_gameObjects, [gameObject](const std::unique_ptr<GameObject>& go)
            {
                // 나 자신이거나, 자손이면 삭제
                return (go.get() == gameObject) || go->GetTransform()->IsDescendantOf(gameObject->GetTransform());
            });
        // 인덱스 전체 재정비
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(m_gameObjects.size()); ++i)
        {
            m_gameObjects[i]->m_sceneIndex = i;
        }
    }

    void Scene::Save()
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

        std::filesystem::path path{ "Resource/Scene" };
        path /= (m_name + ".json");

        if (path.has_parent_path())
        {
            std::error_code ec;
            if (!std::filesystem::create_directories(path.parent_path(), ec))
            {
                FATAL_CHECK(ec.value() == 0, ec.message());
            }
        }

        std::ofstream o(path);
        if (o.is_open())
        {
            o << std::setw(4) << root << std::endl;
        }
    }

    void Scene::SaveToJson(json& outJson)
    {
        outJson["Name"] = m_name;
        outJson["NumGameObjects"] = m_gameObjects.size();
        outJson["GameObjects"] = json::array();

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
            outJson["GameObjects"].push_back(goJson);
        }
    }

    void Scene::Load()
    {
        std::filesystem::path path{ "Resource/Scene" };
        path /= (m_name + ".json");

        std::ifstream i(path);
        if (!i.is_open())
        {
            FATAL_CHECK(false, path.string());
            return;
        }

        json root;
        i >> root;

        Clear();

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

        for (auto go : m_gameObjectAddList)
        {
            if (go->GetTransform()->GetParent() == nullptr)
            {
                go->UpdateActiveInHierarchy(go->IsActiveSelf());
            }
        }
    }

    void Scene::LoadFromJson(const json& inJson)
    {
        Clear();

        JsonGet(inJson, "Name", m_name);
        size_t numGameObjects;
        JsonGet(inJson, "NumGameObjects", numGameObjects);


        std::vector<GameObject*> idToPtr(numGameObjects + 1);
        std::vector<std::pair<int, int>> parentLinks;

        JsonArrayForEach(inJson, "GameObjects", [&](const json& goJson)
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

        for (auto go : m_gameObjectAddList)
        {
            if (go->GetTransform()->GetParent() == nullptr)
            {
                go->UpdateActiveInHierarchy(go->IsActiveSelf());
            }
        }
    }
}