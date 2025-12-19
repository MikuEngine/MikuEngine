#pragma once

namespace engine
{
    template <typename T>
    class System
    {
    protected:
        std::vector<T*> m_components;

    public:
        virtual ~System() = default;

    public:
        virtual void Register(T* component)
        {
            if (component->m_systemIndex != -1)
            {
                return;
            }

            component->m_systemIndex = static_cast<std::int32_t>(m_components.size());
            m_components.push_back(component);
        }

        virtual void Unregister(T* component)
        {
            std::int32_t index = component->m_systemIndex;

            if (index < 0 || index >= static_cast<std::int32_t>(m_components.size()))
            {
                return;
            }

            T* back = m_components.back();
            m_components[index] = back;
            m_components.pop_back();

            back->m_systemIndex = index;
            component->m_systemIndex = -1;
        }
    };
}