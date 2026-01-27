#pragma once

#include <type_traits>
#include <cassert>
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
    // - Ptr 사용시 무조건 if(ptr)로 검증 후에 사용해야함
    // ═══════════════════════════════════════════════════════════════

    template <typename T>
    class Ptr
    {
    private:
        // 유효성 검증 후 유효하지 않다면 handle을 0, 0 으로 바꿔주기 위해서 mutable 사용함
        mutable Handle m_handle;

    public:
        Ptr() = default;
        Ptr(const Ptr&) = default;
        Ptr(Ptr&&) noexcept = default;
        Ptr(std::nullptr_t) // nullptr로 초기화
            : m_handle{}
        {

        }

        Ptr(Handle handle) // handle로 초기화
            : m_handle{ handle }
        {

        }

        // object 포인터로 초기화
        // 이 초기화는 object가 유효한 상태여야함
        // 유효한지 확실한 경우에만 사용해야함
        Ptr(T* object)
        {
            static_assert(std::is_base_of_v<Object, T>, "Ptr<T>: T must inherit from Object");
            
            if (object != nullptr)
            {
                m_handle = object->GetHandle();
            }
        }

        // 부모, 자식 간의 Ptr 대입
        template <typename U>
            requires std::is_base_of_v<T, U>
        Ptr(const Ptr<U>& other)
            : m_handle{ other.GetHandle() }
        {

        }

        // 복사/이동 대입 연산자
        Ptr& operator=(const Ptr&) = default;
        Ptr& operator=(Ptr&&) noexcept = default;

        // 대입 연산자들
        Ptr& operator=(std::nullptr_t)
        {
            m_handle = {};

            return *this;
        }

        Ptr& operator=(Handle handle)
        {
            m_handle = handle;
            return *this;
        }

        // object 포인터로 대입 연산
        // 이 초기화는 object가 유효한 상태여야함
        // 유효한지 확실한 경우에만 사용해야함
        Ptr& operator=(T* object)
        {
            static_assert(std::is_base_of_v<Object, T>, "Ptr<T>: T must inherit from Object");
            
            if (object != nullptr)
            {
                m_handle = object->GetHandle();
            }
            else
            {
                m_handle = {};
            }

            return *this;
        }

        // 부모, 자식 간의 Ptr 대입
        template <typename U>
            requires std::is_base_of_v<T, U>
        Ptr& operator=(const Ptr<U>& other)
        {
            m_handle = other.GetHandle();

            return *this;
        }

        // Handle 접근
        Handle GetHandle() const { return m_handle; }

        // 역참조 연산자
        T* operator->() const
        {
            return Get();
        }

        // 역참조 연산자 (참조 반환)
        T& operator*() const
        {
            // 여기서는 if (ptr)로 검증했다고 간주하고 그냥 바로 포인터 리턴함.
            return *Get();
        }

        // 포인터 얻기
        T* Get() const
        {
            // 여기서는 if (ptr)로 검증했다고 간주하고 그냥 바로 포인터 리턴함.
            return static_cast<T*>(GetObjectFromHandle(m_handle));
        }

        // bool 변환 (유효성 검사)
        operator bool() const
        {
            if (!m_handle.IsValid())
            {
                return false;
            }

            if (GetObjectFromHandle(m_handle) == nullptr)
            {
                m_handle = Handle{};

                return false;
            }

            return true;
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
