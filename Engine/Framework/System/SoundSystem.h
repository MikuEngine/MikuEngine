#pragma once

#include "fmod.hpp"
#include "fmod_errors.h"
#include <map>
#include <string>
#include <vector>
#include <list>
#include <DirectXMath.h>
#include "Framework/Object/Component/AudioSource.h"
#include "Framework/System/System.h"

using namespace DirectX;
namespace fs = std::filesystem;

namespace engine
{
    // ==============================================================
    // Sound Class Implementation
    // ==============================================================

	inline FMOD_VECTOR ToFmodVector(const XMFLOAT3& v) { return { v.x, v.y, v.z }; }

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
        std::string    m_name;
        int            m_id;

    public:
        Sound(FMOD::System* system, int index, std::string name);
        ~Sound();

        void Release();

        void Play2D(bool bLoop = false, EventEndPlay callback = nullptr);

        void Play3D(const XMFLOAT3& position, bool bLoop = false);

        void Update3DPosition(const XMFLOAT3& position);

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
        SoundSystem();
        virtual ~SoundSystem();

    private:
        FMOD::System* m_pSystem = nullptr;
        std::map<std::string, Sound*> m_soundResources; // 리소스 캐싱

        // FMOD 리스너(듣는 사람) 정보
        XMFLOAT3 m_listenerPos = { 0, 0, 0 };
        XMFLOAT3 m_listenerForward = { 0, 0, 1 };
        XMFLOAT3 m_listenerUp = { 0, 1, 0 };

        // 콜백 처리용
        std::list<SoundCallbackInfo> m_callbackList;

        std::vector<std::string> m_PlayUIList;
        const std::string m_soundPath = "../../Resource/Sound/";
        int m_selectedSoundIndex = 0;
        int m_index = 0;

    public:
        bool Initialize();
        void Shutdown();

        bool Release();

        void Register(AudioSource* source) override;
        void Unregister(AudioSource* source) override;

        void Update();

        Sound* GetOrLoadSound(const std::string& filename, bool is3D);

        void RefreshSoundList();

        // FMOD Listener 설정 (CameraSystem에서 Main Camera 정보를 받아와서 호출해줘야 함)
        void SetListenerAttributes(const XMFLOAT3& pos, const XMFLOAT3& forward, const XMFLOAT3& up);

        void DrawImgui();
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