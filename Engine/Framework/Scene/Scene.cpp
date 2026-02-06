#include "EnginePCH.h"
#include "Scene.h"

#include <functional>
#include <fstream>
#include <algorithm>

#include "Common/Utility/JsonHelper.h"
#include "Core/System/VirtualFileSystem.h"
#include "Core/System/VirtualFileSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Camera.h"
#include "Framework/Object/Component/Transform.h"
#include <Framework/Object/Component/RectTransform.h>
#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/CharacterController.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/Physics/PhysicsCallback.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionSystem.h"

#include <PxPhysicsAPI.h>

namespace engine
{
    // unique_ptr<PhysicsEventCallback>이 완전한 타입을 알아야 하므로 .cpp에서 정의
    Scene::Scene() = default;
    
    Scene::~Scene()
    {
        // PhysX 리소스 정리 (애플리케이션 종료 시 누수 방지)
        ClearPhysicsScene();
    }
    
    Scene::Scene(Scene&&) noexcept = default;
    Scene& Scene::operator=(Scene&&) noexcept = default;

    GameObject* Scene::CreateGameObject(const std::string& name)
    {
        return CreateGameObject(CreateObjectType::Default, name);
    }

    GameObject* Scene::CreateGameObject(CreateObjectType type, const std::string& name)
    {
        m_incubator.push_back(std::make_unique<GameObject>());

        GameObject* ptr = m_incubator.back().get();
        ptr->m_name = name;
        ptr->m_sceneIndex = static_cast<int32_t>(m_gameObjects.size() - 1);

        switch (type)
        {
        case CreateObjectType::UI:

            ptr->ReplaceTransformWithRectTransform();
            break;
        }

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

    GameObject* Scene::FindGameObject(std::string_view name)
    {
        for (const auto& gameObject : m_gameObjects)
        {
            if (gameObject->m_name == name)
            {
                return gameObject.get();
            }
        }

        // 생성 직후/씬 로드 직후의 오브젝트도 탐색함.
        for (const auto& gameObject : m_incubator)
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

    void Scene::Clear(bool preservePersistent)
    {
        // DontDestroyOnLoad 객체의 물리 컴포넌트 참조 보존
        std::vector<Rigidbody*> preservedRigidbodies;
        std::vector<Collider*> preservedColliders;
        std::vector<CharacterController*> preservedControllers;

        if (preservePersistent)
        {
            for (auto& gameObject : m_gameObjects)
            {
                if (gameObject->IsDontDestroyOnLoad())
                {
                    // 물리 컴포넌트 참조 보존
                    if (Rigidbody* rb = gameObject->GetComponent<Rigidbody>())
                    {
                        preservedRigidbodies.push_back(rb);
                    }
                    if (Collider* col = gameObject->GetComponent<Collider>())
                    {
                        preservedColliders.push_back(col);
                    }
                    if (CharacterController* ctrl = gameObject->GetComponent<CharacterController>())
                    {
                        preservedControllers.push_back(ctrl);
                    }
                    
                    m_dontDestroyGameObjects.push_back(std::move(gameObject));
                }
            }
        }

        m_gameObjects.clear();

        m_gameObjectKillList.clear();
        m_componentKillList.clear();
        m_morgue.clear();

        m_incubator.clear();
        m_gameObjectAddList.clear();
        m_componentAddList.clear();
        m_componentAddProcList.clear();

        // 물리 데이터 정리 (DontDestroyOnLoad 객체 제외)
        m_rigidbodies = std::move(preservedRigidbodies);
        m_colliders = std::move(preservedColliders);
        m_controllers = std::move(preservedControllers);
        
        // 이벤트 큐 클리어
        m_pendingCollisionEvents.clear();
        m_pendingTriggerEvents.clear();
        
        // 시뮬레이션 상태 초기화
        m_physicsAccumulator = 0.0f;
        m_isSimulating = false;
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

        if (!m_dontDestroyGameObjects.empty())
        {
            for (auto& gameObject : m_dontDestroyGameObjects)
            {
                m_gameObjects.push_back(std::move(gameObject));
            }

            m_dontDestroyGameObjects.clear();
        }

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

    void Scene::ReorderGameObjectEditor(GameObject* gameObject, int newIndex)
    {
        if (!gameObject)
        {
            return;
        }

        // 현재 인덱스 찾기
        int currentIndex = -1;
        for (size_t i = 0; i < m_gameObjects.size(); ++i)
        {
            if (m_gameObjects[i].get() == gameObject)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        if (currentIndex == -1)
        {
            return; // 게임오브젝트를 찾을 수 없음
        }

        // 새 인덱스 범위 검사
        if (newIndex < 0)
        {
            newIndex = 0;
        }
        else if (newIndex >= static_cast<int>(m_gameObjects.size()))
        {
            newIndex = static_cast<int>(m_gameObjects.size()) - 1;
        }

        // 같은 위치면 변경 불필요
        if (currentIndex == newIndex)
        {
            return;
        }

        // 순서 변경: 현재 요소를 임시로 저장하고 제거한 후 새 위치에 삽입
        std::unique_ptr<GameObject> temp = std::move(m_gameObjects[currentIndex]);
        
        if (currentIndex < newIndex)
        {
            // 뒤로 이동: 현재 위치부터 새 위치까지 앞으로 shift
            for (int i = currentIndex; i < newIndex; ++i)
            {
                m_gameObjects[i] = std::move(m_gameObjects[i + 1]);
            }
        }
        else
        {
            // 앞으로 이동: 새 위치부터 현재 위치까지 뒤로 shift
            for (int i = currentIndex; i > newIndex; --i)
            {
                m_gameObjects[i] = std::move(m_gameObjects[i - 1]);
            }
        }
        
        m_gameObjects[newIndex] = std::move(temp);

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
        path /= (m_name + ".scene");

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

    void Scene::Load(std::function<void(float)> progressCallback)
    {
        std::filesystem::path path{ "Resource/Scene" };
        path /= (m_name + ".scene");
        std::string scenePath = path.generic_string();

        // VFS를 통해 파일 로드 시도
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        
        bool loaded = vfs.LoadFile(scenePath, fileData);
        
        json root;
        
        if (loaded)
        {
            try
            {
                root = json::parse(fileData.begin(), fileData.end());
            }
            catch (const json::parse_error& e)
            {
                LOG_ERROR("Scene 파일 파싱 실패: {} - Scene", e.what());
                FATAL_CHECK(false, scenePath);
                return;
            }
        }
        else
        {
            // VFS에서 실패하면 파일 시스템에서 시도 (개발 모드)
            std::ifstream i(path);
            if (!i.is_open())
            {
                FATAL_CHECK(false, path.string());
                return;
            }
            i >> root;
        }

        JsonGet(root, "Name", m_name);
        size_t numGameObjects;
        JsonGet(root, "NumGameObjects", numGameObjects);


        std::vector<GameObject*> idToPtr(numGameObjects + 1);
        std::vector<std::pair<int, int>> parentLinks;
        size_t loadedCount = 0;

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

                ++loadedCount;
                if (progressCallback)
                {
                    progressCallback(numGameObjects > 0 ? static_cast<float>(loadedCount) / static_cast<float>(numGameObjects) : 1.f);
                }

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
                    // 씬 로드 시에는 저장된 Local 좌표를 유지 (worldPositionStays = false)
                    c->GetTransform()->SetParent(p->GetTransform(), false);
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

        if (progressCallback)
        {
            progressCallback(1.0f);
        }
    }

    void Scene::LoadFromJson(const json& inJson)
    {
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
                    // 씬 로드 시에는 저장된 Local 좌표를 유지 (worldPositionStays = false)
                    c->GetTransform()->SetParent(p->GetTransform(), false);
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

    // ═══════════════════════════════════════════════════════════════
    // 물리 시스템 인터페이스
    // ═══════════════════════════════════════════════════════════════

    void Scene::StartPhysics()
    {
        SystemManager::Get().GetPhysicsSystem().CreateScenePhysics();
    }

    void Scene::StopPhysics()
    {
        SystemManager::Get().GetCollisionSystem().ClearActivePairs();
        ClearPhysicsScene();
    }

    void Scene::CreatePhysicsScene(physx::PxScene* pxScene, physx::PxControllerManager* controllerManager)
    {
        m_pxScene = pxScene;
        m_controllerManager = controllerManager;
        
        if (m_pxScene && !m_physicsEventCallback)
        {
            m_physicsEventCallback = std::make_unique<PhysicsEventCallback>();
            m_pxScene->setSimulationEventCallback(m_physicsEventCallback.get());
        }
    }

    void Scene::ClearPhysicsScene()
    {
        // 물리 컴포넌트 컨테이너 정리
        m_rigidbodies.clear();
        m_colliders.clear();
        m_controllers.clear();

        // 이벤트 큐 정리
        m_pendingCollisionEvents.clear();
        m_pendingTriggerEvents.clear();

        // PhysX 리소스 해제
        if (m_controllerManager)
        {
            m_controllerManager->release();
            m_controllerManager = nullptr;
        }

        if (m_pxScene)
        {
            m_pxScene->release();
            m_pxScene = nullptr;
        }
        
        m_physicsEventCallback.reset();

        // 시뮬레이션 상태 초기화
        m_physicsAccumulator = 0.0f;
        m_isSimulating = false;

        LOG_PRINT("[Scene] Cleared physics scene");
    }

    void Scene::RegisterRigidbody(Rigidbody* rb)
    {
        if (!rb) return;

        // 중복 체크
        auto it = std::find(m_rigidbodies.begin(), m_rigidbodies.end(), rb);
        if (it != m_rigidbodies.end()) return;

        m_rigidbodies.push_back(rb);

        // PxActor를 Scene에 추가
        physx::PxRigidActor* actor = rb->GetPxActor();
        if (actor && m_pxScene)
        {
            m_pxScene->addActor(*actor);
        }
    }

    void Scene::UnregisterRigidbody(Rigidbody* rb)
    {
        if (!rb) return;

        // PxActor를 Scene에서 제거
        physx::PxRigidActor* actor = rb->GetPxActor();
        if (actor && m_pxScene)
        {
            m_pxScene->removeActor(*actor);
        }

        // 컨테이너에서 제거 (swap and pop)
        auto it = std::find(m_rigidbodies.begin(), m_rigidbodies.end(), rb);
        if (it != m_rigidbodies.end())
        {
            *it = m_rigidbodies.back();
            m_rigidbodies.pop_back();
        }
    }

    void Scene::RegisterCollider(Collider* collider)
    {
        if (!collider) return;

        // 중복 체크
        auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
        if (it != m_colliders.end()) return;

        m_colliders.push_back(collider);
    }

    void Scene::UnregisterCollider(Collider* collider)
    {
        if (!collider) return;

        // 컨테이너에서 제거
        auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
        if (it != m_colliders.end())
        {
            *it = m_colliders.back();
            m_colliders.pop_back();
        }
    }

    void Scene::RegisterController(CharacterController* controller)
    {
        if (!controller) return;

        auto it = std::find(m_controllers.begin(), m_controllers.end(), controller);
        if (it != m_controllers.end()) return;

        m_controllers.push_back(controller);
    }

    void Scene::UnregisterController(CharacterController* controller)
    {
        if (!controller) return;

        auto it = std::find(m_controllers.begin(), m_controllers.end(), controller);
        if (it != m_controllers.end())
        {
            *it = m_controllers.back();
            m_controllers.pop_back();
        }
    }

    void Scene::QueueCollisionEvent(const CollisionEvent& event)
    {
        m_pendingCollisionEvents.push_back(event);
    }

    void Scene::QueueTriggerEvent(const TriggerEvent& event)
    {
        m_pendingTriggerEvents.push_back(event);
    }

    void Scene::ClearPendingEvents()
    {
        m_pendingCollisionEvents.clear();
        m_pendingTriggerEvents.clear();
    }

    void Scene::OnColliderDestroyed(Collider* collider)
    {
        if (!collider) return;

        Handle colliderHandle = collider->GetHandle();

        // 대기 중인 이벤트에서 제거
        m_pendingCollisionEvents.erase(
            std::remove_if(m_pendingCollisionEvents.begin(), m_pendingCollisionEvents.end(),
                [collider](const CollisionEvent& e)
                {
                    return e.colliderA.Get() == collider || e.colliderB.Get() == collider;
                }),
            m_pendingCollisionEvents.end()
        );

        m_pendingTriggerEvents.erase(
            std::remove_if(m_pendingTriggerEvents.begin(), m_pendingTriggerEvents.end(),
                [collider](const TriggerEvent& e)
                {
                    return e.trigger.Get() == collider || e.other.Get() == collider;
                }),
            m_pendingTriggerEvents.end()
        );

        // Collider 컨테이너에서 제거
        UnregisterCollider(collider);
    }
}