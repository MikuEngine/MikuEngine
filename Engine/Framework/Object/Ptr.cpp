#include "EnginePCH.h"
#include "Ptr.h"
#include "Object.h"

namespace engine
{
    Handle GetHandleFromObject(Object* obj)
    {
        if (obj)
        {
            return obj->GetHandle();
        }
        return Handle{};
    }

    Object* GetObjectFromHandle(Handle handle)
    {
        return Object::GetObjectFromHandle(handle);
    }
}
