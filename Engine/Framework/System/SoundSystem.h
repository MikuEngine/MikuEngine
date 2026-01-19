#pragma once

#include "fmod.hpp"
#include "fmod_errors.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <functional>
#include <DirectXMath.h>

using namespace DirectX;
namespace fs = std::filesystem;
using EventSoundEnd = std::function<void()>;

namespace engine
{
	inline FMOD_VECTOR ToFmodVector(const XMFLOAT3& v) { return { v.x, v.y, v.z }; }

	struct SoundCallbackInfo
	{
		FMOD::Channel* pChannel;
		EventSoundEnd callback;
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

        void Play2D(bool bLoop = false, EventSoundEnd callback = nullptr);

        void Play3D(const XMFLOAT3& position, bool bLoop = false);

        void Update3DPosition(const XMFLOAT3& position);

        void Stop();
        void SetVolume(float vol);
    };

    class SoundManager
    {
    public:
        static SoundManager& GetInstance() { static SoundManager instance; return instance; }

        bool Init();

        void Update(const XMFLOAT3& camPos, const XMFLOAT3& camForward, const XMFLOAT3& camUp);

        bool Release();

        Sound* LoadSound(std::string filename, bool is3D = false);
        Sound* GetSound(std::string filename);


        void RefreshSoundList();

    private:
        SoundManager();
        ~SoundManager();

        FMOD::System* m_pSystem = nullptr;
        std::map<std::string, Sound*> m_SoundList;
        std::vector<std::string> m_PlayUIList;
        int m_selectedSoundIndex = 0;
        int m_index = 0;
        const std::string m_soundPath = "../Resource/sound/";

        // event
        std::list<SoundCallbackInfo> m_callbackList;

    public:
        void DrawImgui();
        void AddCallback(const SoundCallbackInfo& info) { m_callbackList.push_back(info); }

    };

}

/* sound event 사용 예시

재생 후 로그 출력
attackSound->Play(false, [](){
    std::cout << "공격 소리 끝!" << std::endl;
});

재생 후 오브젝트 삭제
dieSound->Play(false, [this](){
    GameObject::Destroy(this->GetGameObject());
});

*/