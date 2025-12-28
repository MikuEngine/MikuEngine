#pragma once

#include "Framework/Object/Object.h"

namespace engine
{
    class GameObject;
    class Transform;

    class Component :
        public Object
    {
    private:
        GameObject* m_owner = nullptr;
        std::int32_t m_systemIndex = -1;

    public:
        GameObject* GetGameObject() const;
        Transform* GetTransform() const;

    private:
        template <typename T>
        friend class System;
        friend class GameObject;
    };
}