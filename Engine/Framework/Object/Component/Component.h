#pragma once

#include "Framework/Object/Object.h"

namespace engine
{
    namespace detail
    {
        inline int GetNextComponentTypeID()
        {
            static int s_nextID = 0;
            return s_nextID++;
        }
    }

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
        bool m_hasAwoken = false;

    public:
        GameObject* GetGameObject() const;
        Transform* GetTransform() const;

        bool IsActive() const override;
        void SetActive(bool active) override;
        void MarkAsAwoken();
        bool HasAwoken() const;

        virtual void Initialize() {}
        virtual void Awake() {} // Initialize 직후 호출
        void Destroy();
        virtual void OnDestroy() {};
        bool IsPendingKill() const;

        std::int32_t GetSystemIndex() const;
        void SetSystemIndex(std::int32_t index);

    public: // 타입 체크 관련 함수들
        static int GetStaticTypeID();

        virtual int GetTypeID() const;
        virtual const char* GetTypeName() const;
        virtual bool IsA(int typeID) const;

        template <typename T>
        bool Is() const
        {
            return IsA(T::GetStaticTypeID());
        }

        template <typename T>
        T* As()
        {
            if (Is<T>())
            {
                return static_cast<T*>(this);
            }

            return nullptr;
        }

        template <typename T>
        const T* As() const
        {
            if (Is<T>())
            {
                return static_cast<const T*>(this);
            }

            return nullptr;
        }

    private:
        template <typename T>
        friend class System;
        friend class GameObject;
    };
}