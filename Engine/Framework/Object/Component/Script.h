#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    enum class ScriptEvent
    {
        Initialize    = 0,
        Start         = 1,
        Update        = 2,
        FixedUpdate   = 3,
        LateUpdate    = 4,
        Count         = 5
    };

    class ScriptBase :
        public Component
    {
    private:
        friend class ScriptSystem;

        std::array<std::int32_t, static_cast<size_t>(ScriptEvent::Count)> m_systemIndices;

    public:
        ScriptBase();
        ~ScriptBase();

    protected:
        void RegisterScript(std::uint32_t eventFlags);

    public:
        virtual void Initialize();
        virtual void Start();
        virtual void Update();

    public:
        void OnGui() override {};
    };

    template <typename Base, typename Derived>
    consteval bool IsFuncOverridden(void (Base::* baseFunc)(), void (Derived::* derivedFunc)())
    {
        return baseFunc != static_cast<void (Base::*)()>(derivedFunc);
    }

    template <typename T>
    class Script :
        public ScriptBase
    {
    public:
        Script()
        {
            std::uint32_t eventFlags = 0;
            
            // 컴파일 타임에 해당 이벤트 함수들 구현되었는지 확인 후
            // 오버라이드 된 함수들만 플래그 켜줌

            if constexpr (IsFuncOverridden(&ScriptBase::Start, &T::Start))
            {
                eventFlags |= 1U << static_cast<int>(ScriptEvent::Start);
            }

            if constexpr (IsFuncOverridden(&ScriptBase::Update, &T::Update))
            {
                eventFlags |= 1U << static_cast<int>(ScriptEvent::Update);
            }

            RegisterScript(eventFlags);
        }

        ~Script() = default;
    };
}