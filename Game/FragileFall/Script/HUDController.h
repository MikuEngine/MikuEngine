#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class GameObject;
    class UIImage;
    class UIProgressBar;
    class RectTransform;
    class UIText;
}

namespace game
{
    class PlayerControllerScript;

    class HUDController :
        public engine::Script<HUDController>
    {
        REGISTER_SCRIPT(HUDController, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        static void UpdateHpSnapshot(float baseMaxHp, float finalMaxHp, float currentHp);
        static void ClearHpSnapshot();

    private:
        void CacheHearts();
        void OnDamagedHalf();
        void TryBindPlayer();
        void RefreshHpSnapshotFromPlayer();
        void ValidateHeartsOnStageEntry();

        int CalcFilledHalfSlotsFromPlayer(int visibleHalfSlots) const;
        int CalcVisibleHalfSlotsFromPlayer() const;
        float CalcHpPerHalfHeart() const;

        void ApplyHearts();

        void SetHeartFull(int i);
        void SetHeartHalf(int i);
        void SetHeartEmpty(int i);

        engine::Vector4 GetHeartFullClip(int i) const;
        engine::Vector4 GetHeartHalfClip(int i) const;

    private:
        PlayerControllerScript* m_playerScript = nullptr;

        engine::UIProgressBar* m_fragileGaugeProgress = nullptr;

        engine::UIImage* m_hitImage = nullptr;
        engine::UIImage* m_fragileImage = nullptr;

        static constexpr int kHeartCount = 8;
        static constexpr int kBaseHeartCount = 5;
        engine::Ptr<engine::UIImage> m_hearts[kHeartCount];
        engine::Ptr<engine::GameObject> m_heartGO[kHeartCount];
        engine::Ptr<engine::GameObject> m_heartCaseGO[kHeartCount];
        engine::Ptr<engine::RectTransform> m_heartRT[kHeartCount];

        int m_filledHalfSlots = 10;
        int m_visibleHalfSlots = 10;
        bool m_forceApplyHearts = true;
        bool m_damageCallbackBound = false;
        bool m_cached = false;

        static bool s_hasHpSnapshot;
        static float s_cachedBaseMaxHp;
        static float s_cachedFinalMaxHp;
        static float s_cachedCurrentHp;
    };
}