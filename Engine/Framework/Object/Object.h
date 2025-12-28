#pragma once

#include "Framework/Object/Handle.h"

namespace engine
{
    class Object
    {
    private:
        Handle m_handle;

    protected:
        Object();
        virtual ~Object();

    public:
        Handle GetHandle() const;
        static Object* GetObjectFromHandle(Handle handle);

    public:
        virtual void OnGui() {}
        virtual void Save(json& j) const {}
        virtual void Load(const json& j) {}
        virtual std::string GetType() const = 0;

    private:
        void RegisterObject(Object* object);
        void UnregisterObject(Object* object);
    };
}