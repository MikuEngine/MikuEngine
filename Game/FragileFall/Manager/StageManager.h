#pragma once

namespace engine
{
    class GameObject;
}

namespace game
{
    class MonsterSpawner;

    // 스테이지 단위 상태 관리, 몬스터 생존 판정, 클리어/보상/문·탈출구 제어, 씬 전환.
    class StageManager :
        public engine::Singleton<StageManager>
    {
    private:
        engine::Vector3 m_doorNextPosition{ 0, 0, 15 };   // (0,0,0)이면 다음 스테이지 문 미생성
        engine::Vector3 m_doorExitPosition{ 0, 0, -15 };  // (0,0,0)이면 탈출구 미생성

        // 런타임 상태
        int m_currentStage = 1;
        std::vector<engine::Ptr<engine::GameObject>> m_spawnedMonsters;
        int m_runRuby = 0;
        int m_runSapphire = 0;
        int m_runEmerald = 0;
        bool m_cleared = false;
        engine::Ptr<engine::GameObject> m_currentMapEnvRoot;
        engine::Ptr<engine::GameObject> m_currentSpawnerRoot;

    private:
        StageManager() = default;
        ~StageManager() = default;

    public:

        /// 로비에서 플레이 진입 시 호출. m_currentStage = 1 후 씬 전환은 호출 측에서.
        void ResetToStage1()
        {
            m_currentStage = 1;
            m_runRuby = m_runSapphire = m_runEmerald = 0;
        }

        /// 플레이 씬에서 스테이지 시작 시 호출 (예: SceneController_Play::Awake).
        /// CSV에서 맵 환경 프리팹 조회, 난이도 세팅, 스포너+장애물 1종 랜덤 → Instantiate → 스포너 설정.
        /// 스폰은 Spawner Start() → OnSpawnerReady → SpawnNow() 로 같은 프레임에 수행.
        void BeginStage();

        /// 스포너 Start()에서 managed 모드일 때 호출. SpawnNow() 후 스폰된 몬스터 Ptr 목록 복사.
        void OnSpawnerReady(MonsterSpawner* spawner);

        /// 매 프레임 호출 권장 (플레이 씬). 클리어 판정 1회 처리, 보상·문/탈출구 활성화.
        void Update();

        /// 프레자일 게이지용. 몬스터가 하나라도 살아 있으면 true.
        bool ShouldFragileGaugeRise() const;

        /// 현재 스테이지 번호 (1부터, 10·20·30… = 보스).
        int GetCurrentStage() const { return m_currentStage; }

        /// 이번 플레이 런에서 번 재화 추가. (몬스터 드롭·클리어 보상 등에서 호출)
        void AddRunCurrency(int ruby, int sapphire, int emerald);

        /// 탈출구 접촉 시. m_currentStage=1 리셋 후 로비 씬으로 전환.
        void RequestGoToLobby();

        /// 다음 맵 문 접촉 시. m_currentStage++ 후 보스면 보스 씬, 아니면 같은 플레이 씬에서 다음 스테이지 생성.
        void RequestNextStage();

        /// 클리어 시 생성할 "다음 스테이지 문" 위치. (0,0,0)이면 생성 안 함.
        void SetDoorNextPosition(const engine::Vector3& pos) { m_doorNextPosition = pos; }
        /// 클리어 시 생성할 "탈출구" 위치. (0,0,0)이면 생성 안 함.
        void SetDoorExitPosition(const engine::Vector3& pos) { m_doorExitPosition = pos; }

    private:
        friend class engine::Singleton<StageManager>;

        void ClearStageState();
        void ComputeDifficulty(int stageIndex, int& targetScore, int& minCount, int& maxCount) const;
        bool GetMapEnvPrefabNameForStage(int stageIndex, std::string& outName) const;
    };
}
