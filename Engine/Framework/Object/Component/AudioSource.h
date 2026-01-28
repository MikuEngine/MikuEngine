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
        std::string m_clipName;                     // 재생할 사운드 파일 키값
        std::shared_ptr<SoundData> m_soundData;     // 로드된 사운드 리소스 데이터
		std::vector<std::string> m_randomClipNames; // 랜덤 후보 리스트
        std::string m_bus = "SFX";                  // 기본 로드 옵션

        bool m_playOnAwake = false;
        bool m_isLoop = false;
        bool m_is3D = false;
        bool m_isPlaying = false;
        bool m_useRandom = false;                   // 랜덤 모드 켜기/끄기
        
        float m_volume = 1.0f;
        float m_minDistance = 1.0f;
        float m_maxDistance = 500.0f;
		int m_lastRandomIndex = -1;                 // 방금 재생한 인덱스 (중복 방지용)

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

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

        void Initialize() override;
        void Update();
        //void Awake() override;
        void OnDestroy() override;

        void Play(EventCallBack callback = nullptr, float fadeInDuration = 0.0f);
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
        void SetBus(std::string busName) { m_bus = busName; }

        Sound* GetSoundResource() const;
        FMOD::Channel* GetChannel() const { return m_currentChannel; }
        std::string GetBus() const { return m_bus; }
        bool Is3D() const { return m_is3D; }
        bool IsPlaying() const { return m_isPlaying; }

        void AddRandomClip(const std::string& path) { m_randomClipNames.push_back(path); }
        void RemoveRandomClip(int index)
        {
            if (index >= 0 && index < m_randomClipNames.size())
                m_randomClipNames.erase(m_randomClipNames.begin() + index);
        }

        void OnSoundEnd();

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
    };
}

/*//
<< AudioSource Component event 사용 예시 >>
** 재생 후 로그 출력 **
attackSound->Play([]() {
    std::cout << "공격 소리 끝!" << std::endl;
    });

** 재생 후 오브젝트 삭제 **
dieSound->Play([this]()
    {
        // 엔진이 안전하다면 아래 코드 사용 가능
        this->m_destroyPending = true;
        // 플래그를 세우거나
        // 엔진이 싱글 스레드 이벤트 처리를 보장한다면
        GameObject::Destroy(this->GetGameObject());

    }, 0.0f); // 두 번째 인자는 fadeInDuration


<< play fadeIn / fadeOut 사용 예시 >>
** Play(float fadeIn, float duration, float fadeOut) 함수 **
// 1.5초 동안 소리가 커짐
// 3.0초 동안 소리 유지
// 1.5초 동안 소리가 줄어들며 종료
// 총 재생 시간: 6초
AudioSource->Play(1.5f, 3.0f, 1.5f);
//*/