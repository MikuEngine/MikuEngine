#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Physics/CollisionTypes.h"

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

    class GameObject;
    class Scene;
    enum class CreateObjectType;

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

        GameObject* CreateGameObject(const std::string& name = "GameObject");
        GameObject* CreateGameObject(CreateObjectType type, const std::string& name);

        // ═══════════════════════════════════════════════════════════════
        // 충돌 콜백 (Push 방식)
        // 필요한 함수만 오버라이드하여 사용
        // ═══════════════════════════════════════════════════════════════
        
        // 물리 충돌 (IsTrigger = false)
        virtual void OnCollisionEnter(const CollisionInfo& info) {}
        virtual void OnCollisionStay(const CollisionInfo& info) {}
        virtual void OnCollisionExit(const CollisionInfo& info) {}
        
        // 트리거 충돌 (IsTrigger = true)
        virtual void OnTriggerEnter(const CollisionInfo& info) {}
        virtual void OnTriggerStay(const CollisionInfo& info) {}
        virtual void OnTriggerExit(const CollisionInfo& info) {}

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

            eventFlags |= 1U << static_cast<int>(ScriptEvent::Start); // start는 한번이니까 그냥 다 호출함

            if constexpr (IsFuncOverridden(&ScriptBase::Update, &T::Update))
            {
                eventFlags |= 1U << static_cast<int>(ScriptEvent::Update);
            }

            RegisterScript(eventFlags);
        }
    };
}