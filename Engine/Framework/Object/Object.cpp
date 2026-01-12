#include "EnginePCH.h"
#include "Object.h"

#include <queue>

namespace engine
{
    namespace
    {
        struct ObjectEntry
        {
            Object* object = nullptr;
            std::uint32_t generation = 0;
            bool active = false;
        };

        std::vector<ObjectEntry> g_objects;
        std::queue<std::uint32_t> g_freeIndices;
    }

    Object::Object()
    {
        RegisterObject(this);
    }

    Object::~Object()
    {
        UnregisterObject(this);
    }

    Handle Object::GetHandle() const
    {
        return m_handle;
    }

    Object* Object::GetObjectFromHandle(Handle handle)
    {
        if (handle.index >= g_objects.size())
        {
            return nullptr;
        }

        if (const auto& entry = g_objects[handle.index];
            (entry.generation == handle.generation) && entry.active)
        {
            return entry.object;
        }

        return nullptr; // 이미 삭제되었거나 재사용된 객체임
    }

    bool Object::IsActive() const
    {
        return m_active;
    }

    void Object::SetActive(bool active)
    {
        if (m_active == active)
        {
            return;
        }

        m_active = active;

        if (m_active)
        {
            OnEnable();
        }
        else
        {
            OnDisable();
        }
    }

    void Object::RegisterObject(Object* object)
    {
        std::uint32_t index = 0;
        std::uint32_t generation = 0;

        if (!g_freeIndices.empty())
        {
            index = g_freeIndices.front();
            g_freeIndices.pop();

            generation = g_objects[index].generation + 1;
        }
        else
        {
            index = static_cast<std::uint32_t>(g_objects.size());
            generation = 1;
            g_objects.push_back({});
        }

        g_objects[index].object = object;
        g_objects[index].generation = generation;
        g_objects[index].active = true;

        m_handle = { index, generation };
    }

    void Object::UnregisterObject(Object* object)
    {
        std::uint32_t index = object->m_handle.index;

        // Handle에 임의 값 할당 되었음. 일어나면 안되는 상황
        assert(object->m_handle.index < g_objects.size());
        assert(g_objects[index].object == object);

        g_objects[index].object = nullptr;
        g_objects[index].active = false;

        g_freeIndices.push(index);
    }

}