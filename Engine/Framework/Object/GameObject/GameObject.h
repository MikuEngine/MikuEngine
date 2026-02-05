#pragma once

#include "Framework/Object/Object.h"
#include "Framework/Object/Component/Component.h"

namespace engine
{
    class Component;
    class Transform;
    class RectTransform;

    class GameObject :
        public Object
    {
    private:
        std::string m_name = "GameObject";
        std::vector<std::unique_ptr<Component>> m_components;
        Transform* m_transform;
        std::int32_t m_sceneIndex = -1;

        bool m_hasAwoken = false;
        bool m_isPendingKill = false;
        bool m_activeInHierarchy = true;
        bool m_isDontDestroyOnLoad = false;

    public:
        GameObject();
        ~GameObject() = default;

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

    public:
        Transform* GetTransform() const;
        const std::string& GetName() const;

        void SetName(const std::string& name);

        bool HasAwoken() const;
        void Awake();
        void Destroy();
        bool IsPendingKill() const;
        void RemoveComponentFast(Component* component);

        void BroadcastOnDestroy();

        bool IsActiveSelf() const;
        bool IsActive() const override;
        void SetActive(bool active) override;
        void UpdateActiveInHierarchy(bool parentActive);

    private:
        void RegisterComponentPendingAdd(Component* component);

    public:
        template <std::derived_from<Component> T, typename... Args>
        T* AddComponent(Args&&... args)
        {
            std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);

            T* ptr = component.get();
            m_components.push_back(std::move(component));

            ptr->m_owner = this;
            ptr->m_gameObjectIndex = static_cast<std::int32_t>(m_components.size() - 1);
            RegisterComponentPendingAdd(ptr);

            return ptr;
        }

        Component* AddComponent(std::unique_ptr<Component>&& component);

        template <std::derived_from<Component> T>
        T* GetComponent()
        {
            for (const auto& component : m_components)
            {
                if (component->Is<T>())
                {
                    return static_cast<T*>(component.get());
                }
            }

            return nullptr;
        }

        template <typename T>
        T* GetInterface()
        {
            for (const auto& component : m_components)
            {
                T* target = dynamic_cast<T*>(component.get());

                if (target != nullptr)
                {
                    return target;
                }
            }

            return nullptr;
        }

        const std::vector<std::unique_ptr<Component>>& GetComponents() const;
        void RemoveComponent(size_t index);

        RectTransform* ReplaceTransformWithRectTransform();
        void DontDestroyOnLoad();
        bool IsDontDestroyOnLoad();

        static GameObject* Find(std::string_view name);

    public:
        virtual void OnGui() {};
        void Save(json& j) const override;
        void Load(const json& j) override;
        const char* GetTypeName() const override;

    private:
        friend class Scene;
    };
}