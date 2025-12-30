#pragma once

#include <functional>

#include "Framework/Object/GameObject/GameObject.h"

namespace engine
{
    class Camera;
    class GameObject;
    class Component;

    class Scene
    {
    protected:
        std::string m_name;
        std::vector<std::unique_ptr<GameObject>> m_gameObjects;

        // 생성 대기열
        std::vector<std::unique_ptr<GameObject>> m_incubator;
        std::vector<GameObject*> m_gameObjectAddList;
        std::vector<Component*> m_componentAddList;

        // 삭제 대기열
        std::vector<GameObject*> m_gameObjectKillList;
        std::vector<Component*> m_componentKillList;
        std::vector<std::unique_ptr<GameObject>> m_morgue;

    public:
        GameObject* CreateGameObject(const std::string& name = "GameObject");
        Camera* GetMainCamera() const;
        const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const;
        const std::string& GetName() const;

        void SetName(std::string_view name);

        GameObject* FindGameObject(const std::string& name);

        void ResetToDefaultScene();
        void Clear();
        void OnPlayStart();

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
    };
}
