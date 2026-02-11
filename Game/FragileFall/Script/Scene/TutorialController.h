#pragma once

#include <Framework/Object/Component/Script.h>
#include <Script/CharacterScript/Player/PlayerControllerScript.h>
#include <Framework/Object/Component/UI/UIClickArea.h>

namespace game
{
    class UIMessageQueue;

    struct TutorialStep
    {
        std::vector<std::string> pages; // 동시에 보여줄 메시지들
    };

    class TutorialController :
        public engine::Script<TutorialController>
    {
        REGISTER_SCRIPT(TutorialController, Script)

    private:
        UIMessageQueue* m_queue = nullptr;
        int m_stepIndex = 0;
        int m_pageIndex = 0;

        bool m_isStateCheck = false;
        bool m_isTimerActive = false;
        float m_stepTimer = 0.0f;
        
        std::set<std::string> m_bindButtonNames;
		std::set<engine::UIClickArea*> m_bindClickAreaNames;

		bool m_isTutorialFinished = false;

        // ─────────────────────────────────────────────
        // step 0
        // ─────────────────────────────────────────────
        engine::Ptr<engine::GameObject> m_nextDoorObject = nullptr;

        // ─────────────────────────────────────────────
        // step 1
        // ─────────────────────────────────────────────
        float m_timer = 0.0f;
        int m_gaugePhase = 0; // 0: 40->70, 1: 70->40, 2: 종료

        // ─────────────────────────────────────────────
        // step 2
        // ─────────────────────────────────────────────
        bool m_isSpawnMonster = false;
        bool m_keepMonsterOnNext = false;
        std::string m_monsterName = "Monster_DullType_Gray";
        engine::Ptr<engine::GameObject> m_spawnedMonster = nullptr;

        // ─────────────────────────────────────────────
        // step 4
        // ─────────────────────────────────────────────
        engine::Ptr<engine::GameObject> m_exitDoorObject = nullptr;

        // ─────────────────────────────────────────────
        // step 5 ~ 9
        // ─────────────────────────────────────────────
		float m_outlineWidth = 4.0f;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void InitializeStep();
        void RefreshStepContext(int index);

        void OnSceneLoaded();

        void BindButton(const std::string& goName, std::function<void()> callback);
        void UnBindAllButtons();
		void SetActiveButton(const std::string& goName, bool active, bool outline);
        void SetupSkillNodesUI(const std::string& goName, bool isChildSetup = false, bool isActive = false);

        void SetIndex(int step, int page)
        {
            m_stepIndex = step;
            m_pageIndex = page;
		}

        void TutorialFinish();

    private:
        PlayerControllerScript* GetPlayer();
        void UpdateGaugeAnimation();

    public:
        // void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

        void ShowPage();
        void Next();
        void Prev();
    };
}