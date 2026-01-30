#pragma once

#include <Framework/Object/Component/Script.h>

#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIImage.h>

namespace game
{
    class UIToastAnimator :
        public engine::Script<UIToastAnimator>
    {
        REGISTER_SCRIPT(UIToastAnimator, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    public:
        void PlayEnter(const engine::Vector2& targetPos);

        void MoveTo(const engine::Vector2& targetPos, float durationOverride = -1.0f);

        void FadeIn(float durationOverride = -1.0f);
        void FadeOut(float durationOverride = -1.0f);

        bool IsAnimating() const { return m_animating; }
        bool IsFadingOut() const { return m_fading; }
        bool IsFinished()  const { return m_finished; }

        void SetText(const std::string& text);

        float m_enterAlpha = 1.0f; // Enter 슬라이드와 함께 0->1
        float m_fadeAlpha = 1.0f; // FadeIn/Out용 0->1 또는 1->0

        void ApplyAlphaCombined();
    private:
        static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t);

        void CacheTargets();
        void CaptureBaseAlphaRecursive(engine::Transform* t);

    private:
        engine::RectTransform* m_rt = nullptr;

        // --- Motion ---
        engine::Vector2 m_fromPos{ 0.0f, 0.0f };
        engine::Vector2 m_toPos{ 0.0f, 0.0f };

        float m_time = 0.0f;
        float m_duration = 0.50f;

        bool m_animating = false;

        // --- Toast Enter settings ---
        float m_enterSlideOffsetX = 600.0f; // 오른쪽 밖에서 시작할 X 오프셋
        float m_enterDuration = 1.0f;

        // --- Reflow settings ---
        float m_moveDuration = 1.12f;

        // --- Fade settings ---
        float m_fadeInDuration = 0.5f;
        float m_fadeOutDuration = 0.20f;
        float m_fadeTime = 0.0f;
        bool  m_fadeInOnEnter = false;
        bool  m_fading = false;
        bool m_entering = false;
        bool  m_finished = false;

        bool m_fadeModeIn = true;
        
        struct AlphaTarget
        {
            engine::UIText* text = nullptr;
            engine::UIImage* image = nullptr;
            float baseA = 1.0f;
        };
        std::vector<AlphaTarget> m_alphaTargets;

        bool m_inited = false;
    };
}