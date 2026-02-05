#pragma once

#include <functional>

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Physics/CollisionTypes.h"

// PhysX 전방 선언
namespace physx
{
    class PxScene;
    class PxControllerManager;
}

namespace engine
{
    enum class CreateObjectType
    {
        Default,
        UI,
    };

    class Camera;
    class GameObject;
    class Component;
    class Rigidbody;
    class Collider;
    class CharacterController;
    class PhysicsEventCallback;  // class로 전방 선언 (PhysicsCallback.h와 일치)

    class Scene
    {
    public:
        Scene();
        ~Scene();
        
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) noexcept;
        Scene& operator=(Scene&&) noexcept;

    protected:
        std::string m_name;
        std::vector<std::unique_ptr<GameObject>> m_gameObjects;
        std::vector<std::unique_ptr<GameObject>> m_dontDestroyGameObjects;

        // 생성 대기열
        std::vector<std::unique_ptr<GameObject>> m_incubator;
        std::vector<GameObject*> m_gameObjectAddList;
        std::vector<Component*> m_componentAddList;
        std::vector<Component*> m_componentAddProcList;

        // 삭제 대기열
        std::vector<GameObject*> m_gameObjectKillList;
        std::vector<Component*> m_componentKillList;
        std::vector<std::unique_ptr<GameObject>> m_morgue;

        // ═══════════════════════════════════════
        // 물리 데이터
        // ═══════════════════════════════════════
        physx::PxScene* m_pxScene = nullptr;
        physx::PxControllerManager* m_controllerManager = nullptr;
        std::unique_ptr<PhysicsEventCallback> m_physicsEventCallback;
        
        // 물리 컴포넌트 참조
        std::vector<Rigidbody*> m_rigidbodies;
        std::vector<Collider*> m_colliders;
        std::vector<CharacterController*> m_controllers;
        
        // 충돌 이벤트 큐
        std::vector<CollisionEvent> m_pendingCollisionEvents;
        std::vector<TriggerEvent> m_pendingTriggerEvents;
        
        // 시뮬레이션 상태
        float m_physicsAccumulator = 0.0f;
        bool m_isSimulating = false;

    public:
        GameObject* CreateGameObject(const std::string& name = "GameObject");
        GameObject* CreateGameObject(CreateObjectType type, const std::string& name = "GameObject");
        Camera* GetMainCamera() const;
        const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const;
        const std::string& GetName() const;

        void SetName(std::string_view name);

        GameObject* FindGameObject(std::string_view name);

        void ResetToDefaultScene();
        void Clear(bool preservePersistent = false);

        void RegisterPendingAdd(GameObject* gameObject);
        void RegisterPendingAdd(Component* component);
        void ProcessPendingAdds(bool isPlaying);

        void RegisterPendingKill(GameObject* gameObject);
        void RegisterPendingKill(Component* component);
        void ProcessPendingKills();

        void RemoveGameObjectEditor(GameObject* gameObject);

    public:
        void Save();
        void SaveToJson(json& outJson);

        void Load();
        void LoadFromJson(const json& inJson);

        // ═══════════════════════════════════════
        // 물리 시스템 인터페이스
        // ═══════════════════════════════════════
        
        // 물리 씬 시작/종료 (에디터 Play/Stop 및 씬 로드 시 사용)
        void StartPhysics();
        void StopPhysics();
        
        // 물리 씬 생성/정리 (PhysicsSystem에서 호출)
        void CreatePhysicsScene(physx::PxScene* pxScene, physx::PxControllerManager* controllerManager);
        void ClearPhysicsScene();
        
        // 물리 컴포넌트 등록/해제 (컴포넌트 Awake/OnDestroy에서 호출)
        void RegisterRigidbody(Rigidbody* rb);
        void UnregisterRigidbody(Rigidbody* rb);
        void RegisterCollider(Collider* collider);
        void UnregisterCollider(Collider* collider);
        void RegisterController(CharacterController* controller);
        void UnregisterController(CharacterController* controller);
        
        // 충돌 이벤트 큐 (PhysicsCallback에서 호출)
        void QueueCollisionEvent(const CollisionEvent& event);
        void QueueTriggerEvent(const TriggerEvent& event);
        void ClearPendingEvents();
        
        // 접근자
        physx::PxScene* GetPxScene() const { return m_pxScene; }
        physx::PxControllerManager* GetControllerManager() const { return m_controllerManager; }
        
        const std::vector<Rigidbody*>& GetRigidbodies() const { return m_rigidbodies; }
        const std::vector<Collider*>& GetColliders() const { return m_colliders; }
        const std::vector<CharacterController*>& GetControllers() const { return m_controllers; }
        
        std::vector<CollisionEvent>& GetPendingCollisionEvents() { return m_pendingCollisionEvents; }
        std::vector<TriggerEvent>& GetPendingTriggerEvents() { return m_pendingTriggerEvents; }
        
        // 시뮬레이션 상태
        float& GetPhysicsAccumulator() { return m_physicsAccumulator; }
        bool IsSimulating() const { return m_isSimulating; }
        void SetSimulating(bool simulating) { m_isSimulating = simulating; }
        
        // Collider 정리 (파괴 시)
        void OnColliderDestroyed(Collider* collider);
    };
}
