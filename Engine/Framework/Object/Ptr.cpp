#include "EnginePCH.h"
#include "Ptr.h"
#include "Object.h"

namespace engine
{
    Object* GetObjectFromHandle(Handle handle)
    {
        return Object::GetObjectFromHandle(handle);
    }
}
