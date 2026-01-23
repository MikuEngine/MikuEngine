#pragma once

#include <map>
#include <list>
#include "Framework/Object/Component/AudioSource.h"
#include "Framework/System/System.h"
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
		EventEndPlay callback = nullptr;
	};

    class Sound
    {
    public:
        FMOD::System* m_pSystem = nullptr;
        FMOD::Sound* m_pSound = nullptr;
        FMOD::Channel* m_pChannel = nullptr;
        FMOD::ChannelGroup* m_pChannelGroup = nullptr;
        std::string m_name;
        int m_id;

    public:
        Sound(FMOD::System* system, int index, std::string name, FMOD::ChannelGroup* channelGroup);
        ~Sound();

        void Release();

        FMOD::Channel* Play2D(bool bLoop = false, EventEndPlay callback = nullptr);

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

    private:
        SoundSystem() = default;
        virtual ~SoundSystem() = default;

    private:
        FMOD::System* m_pSystem = nullptr;

        std::map<std::string, FMOD::ChannelGroup *> m_channelGroups;

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

    public:
        bool Initialize();
        void Shutdown();

        void Register(AudioSource* source) override;
        void Unregister(AudioSource* source) override;

        void Update();
        void StopAll();
        void OnGameStart();

        Sound* CreateSound(const std::string& filename, const std::string& option);
        const std::string& GetSoundPath() const { return m_soundPath; }

        void RefreshSoundList();

        // FMOD Listener 설정 (CameraSystem에서 Main Camera 정보를 받아와서 호출해줘야 함)
        void SetListenerAttributes(const Vector3& pos, const Vector3& forward, const Vector3& up);

        void DrawImgui();

    private:
        FMOD::ChannelGroup *GetOrCreateChannelGroup(const std::string &groupName);
    };
}

/*// sound event 사용 예시
**재생 후 로그 출력**
attackSound->Play(false, [](){
    std::cout << "공격 소리 끝!" << std::endl;
});

**재생 후 오브젝트 삭제**
dieSound->Play(false, [this](){
    GameObject::Destroy(this->GetGameObject());
});
//*/