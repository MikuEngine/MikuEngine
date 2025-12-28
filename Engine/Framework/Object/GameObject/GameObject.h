#pragma once

#include "Framework/Object/Object.h"
#include "Framework/Object/Component/Component.h"

namespace engine
{
    class Component;
    class Transform;

    class GameObject :
        public Object
    {
    private:
        std::string m_name;
        std::vector<std::unique_ptr<Component>> m_components;
        Transform* m_transform;

    public:
        GameObject(const std::string& name = "GameObject");
        ~GameObject() = default;

    public:
        Transform* GetTransform() const;
        const std::string& GetName() const;

        void SetName(const std::string& name);

    public:
        template <std::derived_from<Component> T, typename... Args>
        T* AddComponent(Args&&... args)
        {
            std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);

            component->m_owner = this;
            T* ptr = component.get();

            m_components.push_back(std::move(component));

            return ptr;
        }

        Component* AddComponent(std::unique_ptr<Component>&& component);

        template <std::derived_from<Component> T>
        T* GetComponent()
        {
            for (const auto& component : m_components)
            {
                if (T* casted = dynamic_cast<T*>(component.get()); casted != nullptr)
                {
                    return casted;
                }
            }

            return nullptr;
        }

        const std::vector<std::unique_ptr<Component>>& GetComponents() const;
        void RemoveComponent(size_t index);

    public:
        virtual void OnGui() {};
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;
    };
}