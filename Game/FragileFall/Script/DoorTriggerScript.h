#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class DoorTriggerScript :
        public engine::Script<DoorTriggerScript>
    {
        REGISTER_SCRIPT(DoorTriggerScript, Script)

    private:
        std::string m_doorObjectName;
        std::string m_nextSceneName;
        bool m_isActive = false;

        engine::Ptr<engine::GameObject> m_doorNextObject = nullptr;
        engine::Vector3 m_doorPosition = { 0, 0, 0 };

        engine::EventCallBack m_onTriggered = nullptr;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;


        void SetActivateDoor(bool active);
		void SetDoorObjectName(const std::string& doorName) { m_doorObjectName = doorName; }
		void SetNextSceneName(const std::string& sceneName) { m_nextSceneName = sceneName; }

        void SetEventCallBack(engine::EventCallBack callback) { m_onTriggered = callback; }

        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        //void OnLevelLoaded() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}