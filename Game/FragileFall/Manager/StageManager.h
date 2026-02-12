#pragma once
#include <algorithm>

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
        bool m_waitingForStageClearExecution = false;
        engine::Ptr<engine::GameObject> m_currentMapEnvRoot;  // 맵 환경(장애물·스폰 포인트 포함) 루트
        engine::Ptr<engine::GameObject> m_stageClearExecutionTarget;

        /// 스테이지 간 유지되는 프레자일 게이지 값
        float m_savedFragileGaugeCurrent = 0.0f;

        /// 스테이지 시작 시(스폰 직후) 미리 정해 둔 클리어 보상. 클리어 시 m_run*에 더한 뒤 0으로 비움.
        int m_stageClearRewardRuby = 0;
        int m_stageClearRewardSapphire = 0;
        int m_stageClearRewardEmerald = 0;

        // 플레이어 현재 체력 보관
        float m_runHp = 100.0f;
        float m_runHpMax = 100.0f;

    private:
        StageManager() = default;
        ~StageManager() = default;

    public:
        void ResetRunHp(float hpMax = 100.0f)
        {
            m_runHpMax = std::max(1.0f, hpMax);
            m_runHp = m_runHpMax;
        }
        float GetRunHP() const { return m_runHp; }
        void SetRunHP(float hp) { m_runHp = std::clamp(hp, 0.0f, m_runHpMax); }

        /// 로비에서 플레이 진입 시 호출. m_currentStage = 1 후 씬 전환은 호출 측에서.
        void ResetToStage1()
        {
            m_currentStage = 1;
            m_runRuby = m_runSapphire = m_runEmerald = 0;
            ResetFragileGauge();  // 새 플레이 시작 시 프레자일 게이지 초기화
        }

        /// 플레이 씬에서 스테이지 시작 시 호출 (예: SceneController_Play::Awake).
        /// CSV에서 맵 환경 프리팹 조회 후 Instantiate. 맵 프리팹에 장애물·스폰 포인트·MonsterSpawner 포함.
        /// 스폰은 Spawner Start() → OnSpawnerReady → SpawnNow() 로 같은 프레임에 수행.
        void BeginStage();

        /// 스포너 Start()에서 managed 모드일 때 호출. SpawnNow() 후 스폰된 몬스터 Ptr 목록 복사.
        void OnSpawnerReady(MonsterSpawner* spawner);

        /// 매 프레임 호출 권장 (플레이 씬). 클리어 판정 1회 처리, 보상·문/탈출구 활성화.
        void Update();

        /// 프레자일 게이지용. 몬스터가 하나라도 살아 있으면 true.
        bool ShouldFragileGaugeRise() const;

        /// 프레자일 게이지 관리
        void SaveFragileGauge(float currentGauge) { m_savedFragileGaugeCurrent = currentGauge; }
        float GetSavedFragileGauge() const { return m_savedFragileGaugeCurrent; }
        void ResetFragileGauge() { m_savedFragileGaugeCurrent = 0.0f; }

        /// 현재 스테이지 번호 (1부터, 10·20·30… = 보스).
        int GetCurrentStage() const { return m_currentStage; }

        /// 이번 플레이 런에서 번 재화 추가. (스테이지 클리어 보상은 내부에서 자동 지급, 추가 보상용)
        void AddRunCurrency(int ruby, int sapphire, int emerald);

        int GetRunRuby() const { return m_runRuby; }
        int GetRunSapphire() const { return m_runSapphire; }
        int GetRunEmerald() const { return m_runEmerald; }

        /// 탈출구 접촉 시. m_currentStage=1 리셋 후 로비 씬으로 전환.
        void RequestGoToLobby();

        /// 다음 맵 문 접촉 시. m_currentStage++ 후 보스면 보스 씬, 아니면 같은 플레이 씬에서 다음 스테이지 생성.
        void RequestNextStage();

        /// 스테이지 클리어 후 중앙 처형 오브젝트가 처형되었을 때 호출.
        void OnStageClearExecutionTargetExecuted();

        /// 클리어 시 생성할 "다음 스테이지 문" 위치. (0,0,0)이면 생성 안 함.
        void SetDoorNextPosition(const engine::Vector3& pos) { m_doorNextPosition = pos; }
        /// 클리어 시 생성할 "탈출구" 위치. (0,0,0)이면 생성 안 함.
        void SetDoorExitPosition(const engine::Vector3& pos) { m_doorExitPosition = pos; }

        /// 스포너가 MonsterData.csv 재화 범위로 계산한 클리어 보상을 설정할 때 호출.
        void SetStageClearReward(int ruby, int sapphire, int emerald);

        // 튜토리얼 몬스터 전용
        void RegisterTutorialMonster(engine::GameObject* monster);

    private:
        friend class engine::Singleton<StageManager>;

        void ClearStageState();
        bool GetMapEnvPrefabNameForStage(int stageIndex, std::string& outName) const;
        bool SpawnStageClearExecutionTarget();
        void GrantPendingStageClearReward();
    };
}
