#pragma once
#include "fmod.hpp"

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace engine
{
    class Sound;
    class SoundSystem;

    class AudioSource :
        public Component
    {
        REGISTER_COMPONENT(AudioSource)

        friend class SoundSystem;

    private:
        std::string m_clipName;             // 재생할 사운드 파일 키값
        Sound* m_soundResource = nullptr;   // 로드된 사운드 리소스 포인터
        
        bool m_playOnAwake = false;
        bool m_isLoop = false;
        bool m_is3D = false;
        bool m_isPlaying = false;
        
        float m_volume = 1.0f;
        float m_minDistance = 1.0f;
        float m_maxDistance = 500.0f;

        FMOD::Channel* m_currentChannel = nullptr;

    public:
        AudioSource();
        virtual ~AudioSource();

        void Initialize() override;
        //void Awake() override;
        void OnDestroy() override;

        void Play(EventEndPlay callback = nullptr);
        void Stop();
        void LoadClipFromFile();

        void SetClip(std::string name); 
        void SetVolume(float vol);
        void SetLoop(bool loop);
        void Set3D(bool enable);
        void SetForceStopState();
        
        Sound* GetSoundResource() const { return m_soundResource; }
        FMOD::Channel* GetChannel() const { return m_currentChannel; }
        bool Is3D() const { return m_is3D; }
        bool IsPlaying() const { return m_isPlaying; }

        void OnSoundEnd();

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

        std::string GetType() const override { return "AudioSource"; }
    };
}