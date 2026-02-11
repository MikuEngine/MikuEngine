#pragma once

#include <map>
#include <list>
#include "Framework/Object/Component/AudioSource.h"
#include "Framework/System/System.h"
#include "Common/Utility/CommonTypes.h"
#include "fmod.hpp"

namespace engine
{
// ==============================================================
// Sound Class Implementation
// ==============================================================
	inline FMOD_VECTOR ToFmodVector(const Vector3& v) { return { v.x, v.y, v.z }; }

	struct SoundCallbackInfo
	{
		FMOD::Channel* pChannel = nullptr;
        EventCallBack callback = nullptr;
	};

    class Sound
    {
    public:
        FMOD::System* m_pSystem = nullptr;
        FMOD::Sound* m_pSound = nullptr;
        FMOD::Channel* m_pChannel = nullptr;
        FMOD::ChannelGroup* m_pChannelGroup = nullptr;
        std::string m_key;
        std::string m_filePath;

    public:
        Sound(FMOD::System* system, std::string key, std::string filePath, FMOD::ChannelGroup* channelGroup);
        ~Sound();

        void Release();

        FMOD::Channel* Play2D(bool bLoop = false, EventCallBack callback = nullptr);

        FMOD::Channel* Play3D(const Vector3& position, bool bLoop = false);

        void Update3DPosition(const Vector3& position);

        void Stop();
        void SetVolume(float vol);
    };

    // ==============================================================
    // SoundSystem Class Implementation
    // ==============================================================

    class SoundSystem : 
        public System<AudioSource>,
        public Singleton<SoundSystem>
    {
        friend class Singleton<SoundSystem>;
        friend class Sound;
        friend class SoundData;
        friend class GameObject;
		friend class Transform;

    private:
        SoundSystem() = default;
        virtual ~SoundSystem() = default;

    private:
        FMOD::System* m_pSystem = nullptr;

        std::map<std::string, FMOD::ChannelGroup *> m_channelGroups;
        std::map<std::string, std::vector<Sound*>> m_SoundQues;
		std::set<AudioSource*> m_registeredAudioSources;            // 씬에 활성화 된 AudioSource 컴포넌트들

        FMOD::Channel* m_bgmChannel = nullptr;
        std::string m_currentBgmKey = "";

        GameObject* m_listenerTarget = nullptr;

        // FMOD 리스너(듣는 사람) 정보
        Vector3 m_listenerPos = { 0, 0, 0 };
        Vector3 m_listenerForward = { 0, 0, 1 };
        Vector3 m_listenerUp = { 0, 1, 0 };

        // 콜백 처리용
        std::list<SoundCallbackInfo> m_callbackList;

        std::vector<std::string> m_PlayUIList;
        const std::string m_soundPath = "Resource/Sound/";
        int m_selectedSoundIndex = 0;
        int m_index = 0;

        float m_master = 1.0f;
        float m_bgm = 1.0f;
        float m_sfx = 1.0f;
        bool  m_mute = false;

        bool m_showDebugRanges = true;

    public:
        bool Initialize();
        void Shutdown();

        void Register(AudioSource* source) override;
        void Unregister(AudioSource* source) override;

        void Render();
        void Update();
        void StopAll();
        void StopSceneSounds();
        void OnGameStart();

        Sound* CreateSound(const std::string& key, const std::string& filePath, const std::string& option);
        void CreateRandomSound(const std::string& key, const std::vector<std::string>& filePaths, const std::string& option, LifeScope scope);
        const std::string& GetSoundPath() const { return m_soundPath; }

        void RefreshSoundList();

        void Play(const std::string& key, const std::string& option, bool randomPitch = false, float volume = 1.0f, float pitch = 1.0f, LifeScope scope = LifeScope::Scene);
        void PlayUI(const std::string name);
        void PlayBGM(const std::string& key, float fadeDuration = 1.0f);

        // FMOD Listener 설정 (CameraSystem에서 Main Camera 정보를 받아와서 호출해줘야 함)
        void SetListenerAttributes(const Vector3& pos, const Vector3& forward, const Vector3& up);
        void SetMasterVolume(float v);
        void SetBGMVolume(float v);
        void SetSFXVolume(float v);
        void SetMute(bool mute);
		void SetShowDebugSoundColliders(bool show) { m_showDebugRanges = show; }
        void SetListenerTarget(GameObject* target) { m_listenerTarget = target; }

        void ApplyVolumes();
        static float Clamp01(float v);

        GameObject* GetListenerTarget() const { return m_listenerTarget; }
        float GetMasterVolume() const { return m_master; }
        float GetBGMVolume() const { return m_bgm; }
        float GetSFXVolume() const { return m_sfx; }
        bool  IsMuted() const { return m_mute; }


    private:
        FMOD::ChannelGroup* GetOrCreateChannelGroup(const std::string &groupName);
    };
}

/*//
<<SounSystem Play 함수 사용 예시>>
** 랜덤 사용 시 Name 키 값 맞추기 **
SoundSystem::Get().Play("UI_Click_Random");

** 단일 사운드 재생 시 **
SoundSystem::Get().Play("Resource/Sound/Attack.wav", 0.8f); // 볼륨 0.8
//*/