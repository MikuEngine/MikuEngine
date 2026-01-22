#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace FMOD
{
    class Channel;
}

namespace engine
{
    class Sound;
    class SoundSystem;
    class SoundData;

    enum class FadeState
    {
        None,
        FadingIn,
        FadingOut
    };

    class AudioSource :
        public Component
    {
        REGISTER_COMPONENT(AudioSource, Component)

        friend class SoundSystem;

    private:
        std::string m_clipName;                 // 재생할 사운드 파일 키값
        std::shared_ptr<SoundData> m_soundData;  // 로드된 사운드 리소스 데이터

        bool m_playOnAwake = false;
        bool m_isLoop = false;
        bool m_is3D = false;
        bool m_isPlaying = false;
        
        float m_volume = 1.0f;
        float m_minDistance = 1.0f;
        float m_maxDistance = 500.0f;

        // Fade In / Out
        FadeState m_fadeState = FadeState::None;
        float m_fadeTimer = 0.0f;
        float m_fadeDuration = 0.0f;
        float m_startVolume = 0.0f;     // 페이드 시작 시점의 볼륨
        float m_targetVolume = 0.0f;    // 페이드 목표 볼륨
        float m_fadeInTime = 0.0f;      // 기본 페이드 인 시간
        float m_fadeOutTime = 0.0f;     // 기본 페이드 아웃 시간

        // Auto Stop
        bool m_isAutoStop = false;
        float m_sustainTimer = 0.0f;
        float m_sustainDuration = 0.0f;
        float m_scheduledFadeOutDuration = 0.0f;

        FMOD::Channel* m_currentChannel = nullptr;

    public:
        AudioSource();
        virtual ~AudioSource();

        void Initialize() override;
        void Update();
        //void Awake() override;
        void OnDestroy() override;

        void Play(EventEndPlay callback = nullptr, float fadeInDuration = 0.0f);
        void Play(float fadeIn, float duration, float fadeOut);
        void Stop(float fadeOutDuration = 0.0f);
        void LoadClipFromFile();

        void SetClip(std::string name); 
        void SetVolume(float vol);
        void SetLoop(bool loop);
        void Set3D(bool enable);
        void SetForceStopState();
        void SetFadeInTime(float time) { m_fadeInTime = time; }
        void SetFadeOutTime(float time) { m_fadeOutTime = time; }

        Sound* GetSoundResource() const;
        FMOD::Channel* GetChannel() const { return m_currentChannel; }
        bool Is3D() const { return m_is3D; }
        bool IsPlaying() const { return m_isPlaying; }

        void OnSoundEnd();

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
    };
}