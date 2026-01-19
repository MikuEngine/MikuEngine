#pragma once
#include "Framework/Object/Component/Component.h"

namespace engine
{
    class Sound;
    class SoundSystem;

    class AudioSource :
        public Component
    {
        REGISTER_COMPONENT(AudioSource)

    private:
        std::string m_clipName;             // 재생할 사운드 파일 키값
        Sound* m_soundResource = nullptr;   // 로드된 사운드 리소스 포인터
        
        bool m_playOnAwake = false;
        bool m_isLoop = false;
        bool m_is3D = true;
        
        float m_volume = 1.0f;
        float m_minDistance = 1.0f;
        float m_maxDistance = 500.0f;

    public:
        AudioSource() = default;
        ~AudioSource();

        void Initialize() override;
        void Update();

        void Play(EventEndPlay callback = nullptr);
        void Stop();
        void SetClip(std::string name);

        void SetVolume(float vol);
        void SetLoop(bool loop);
        void Set3D(bool enable);
        
        Sound* GetSoundResource() const { return m_soundResource; }
        bool Is3D() const { return m_is3D; }

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

        std::string GetType() const override { return "AudioSource"; }
    };
}