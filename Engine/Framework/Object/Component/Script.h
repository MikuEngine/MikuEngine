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
        std::array<std::int32_t, static_cast<size_t>(ScriptEvent::Count)> m_systemIndices;

    public:
        ScriptBase();
        ~ScriptBase();

    protected:
        void RegisterScript(std::uint32_t eventFlags);

    public:
        void Awake() override {};
        virtual void Start() {};
        virtual void Update() {};

    public:
        void OnGui() override {};

    private:
        friend class ScriptSystem;
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
        void Initialize() override
        {
            std::uint32_t eventFlags = 0;

            // 컴파일 타임에 해당 이벤트 함수들 구현되었는지 확인 후
            // 오버라이드 된 함수들만 플래그 켜줌

            // Awake는 등록했든 말든 Scene에서 일괄 호출

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
    };
}