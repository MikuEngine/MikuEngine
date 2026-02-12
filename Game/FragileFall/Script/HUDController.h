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

    private:
        void CacheHearts();
        void OnDamagedHalf();

        int CalcHalfHPFromPlayer() const;
        int CalcVisibleHeartCountFromPlayer() const;
        int CalcHpPerHeart() const;
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
        engine::Ptr<engine::RectTransform> m_heartRT[kHeartCount];

        int m_halfHp = 10;
        int m_visibleHeartCount = 5;
        bool m_cached = false;
    };
}