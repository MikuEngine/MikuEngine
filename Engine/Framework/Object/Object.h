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

    private:
        void RegisterObject(Object* object);
        void UnregisterObject(Object* object);
    };
}