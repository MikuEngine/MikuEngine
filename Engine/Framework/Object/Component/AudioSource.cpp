#include "EnginePCH.h"
#include "AudioSource.h"
#include "Framework/System/SoundSystem.h"

namespace engine
{
    // ... 생성자/소멸자 등 ...

    void AudioSource::Initialize()
    {
        // 컴포넌트가 초기화될 때 사운드 리소스를 요청해서 가져옴
        if (!m_clipName.empty())
        {
            // SoundSystem에게 "이 파일 좀 줘(없으면 로드해줘)" 요청
            m_soundResource = SoundSystem::GetInstance().GetOrLoadSound(m_clipName, m_is3D);

            // 시작 시 자동 재생 옵션이 켜져 있으면 재생
            if (m_playOnAwake && m_soundResource)
            {
                Play();
            }
        }
    }

    // ... Play 함수 등 ...
}
