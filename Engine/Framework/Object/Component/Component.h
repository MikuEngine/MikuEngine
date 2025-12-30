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
        std::int32_t m_gameObjectIndex = -1;

        bool m_isPendingKill = false;

    public:
        GameObject* GetGameObject() const;
        Transform* GetTransform() const;

        virtual void Initialize() {}
        virtual void Awake() {} // Initialize 직후 호출
        void Destroy();
        virtual void OnDestroy() {};
        bool IsPendingKill() const;

    private:
        template <typename T>
        friend class System;
        friend class GameObject;
    };
}