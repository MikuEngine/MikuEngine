#pragma once

#include <type_traits>
#include "Framework/Object/Handle.h"

namespace engine
{
    class Object;

    // Handle로부터 Object*를 얻는 헬퍼 (Ptr.cpp에서 정의)
    Object* GetObjectFromHandle(Handle handle);

    // ═══════════════════════════════════════════════════════════════
    // Ptr - Object 기반 타입을 위한 안전한 포인터 래퍼
    // Handle 기반으로 동작하여 댕글링 방지
    // 
    // 사용 규칙:
    // - T는 Object를 상속해야 함 (static_assert로 검사)
    // - 전방 선언만으로 Ptr<T> 멤버 선언 가능
    // - Ptr<T>(pointer)로 생성하거나 Get()을 호출하는 cpp에서는
    //   T의 완전한 정의(헤더)가 필요
    // ═══════════════════════════════════════════════════════════════

    template <typename T>
    class Ptr
    {
    private:
        Handle m_handle;

    public:
        // 기본 생성자
        Ptr() = default;

        // nullptr 대입 허용
        Ptr(std::nullptr_t) : m_handle{} {}

        // Handle로부터 생성 (전방 선언만으로 OK)
        Ptr(Handle handle) : m_handle{ handle } {}

        // 포인터로부터 생성
        // ⚠️ 이 생성자를 사용하는 cpp에서 T의 헤더를 include해야 함
        Ptr(T* object)
        {
            static_assert(std::is_base_of_v<Object, T>, 
                "Ptr<T>: T must inherit from Object");
            
            if (object != nullptr)
            {
                m_handle = object->GetHandle();
            }
        }

        // Handle 접근
        Handle GetHandle() const { return m_handle; }

        // 역참조 연산자
        // ⚠️ 이 연산자를 사용하는 cpp에서 T의 헤더를 include해야 함
        T* operator->() const
        {
            return Get();
        }

        // 포인터 얻기
        // ⚠️ 이 함수를 사용하는 cpp에서 T의 헤더를 include해야 함
        T* Get() const
        {
            static_assert(std::is_base_of_v<Object, T>, 
                "Ptr<T>: T must inherit from Object");
            
            if (!m_handle.IsValid())
            {
                return nullptr;
            }
            Object* object = GetObjectFromHandle(m_handle);
            return static_cast<T*>(object);
        }

        // bool 변환 (유효성 검사)
        operator bool() const
        {
            if (!m_handle.IsValid())
            {
                return false;
            }
            return GetObjectFromHandle(m_handle) != nullptr;
        }

        // 비교 연산자
        bool operator==(const Ptr<T>& other) const
        {
            return m_handle == other.m_handle;
        }

        bool operator!=(const Ptr<T>& other) const
        {
            return m_handle != other.m_handle;
        }

        // nullptr 비교
        bool operator==(std::nullptr_t) const
        {
            return !static_cast<bool>(*this);
        }

        bool operator!=(std::nullptr_t) const
        {
            return static_cast<bool>(*this);
        }
    };
}
