#include "GamePCH.h"
#include "TestSound.h"
#include <Framework/Object/Component/AudioSource.h>

namespace game
{
    void SoundTest::Awake()
    {
        /*/ ex
        auto* audio = GetGameObject()->AddComponent<engine::AudioSource>();
    
        if (audio)
        {
            audio->SetClip("drumloop.wav");

            audio->SetLoop(false);
            audio->SetVolume(1.0f);

            audio->Play();
        }
        //*/
    }

    void SoundTest::Start()
    {

    }

    void SoundTest::Update()
    {
    }

    void SoundTest::OnGui()
    {


    }

    void SoundTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SoundTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}