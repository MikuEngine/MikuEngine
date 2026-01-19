#include "EnginePCH.h"
#include "SoundSystem.h"


namespace engine
{
    Sound::Sound(FMOD::System* system, int index, std::string name)
        : m_pSystem(system), m_id(index), m_name(name) {
    }

    Sound::~Sound() {}

    void Sound::Release()
    {
        if (m_pSound) { m_pSound->release(); m_pSound = nullptr; }
    }

    void Sound::Play2D(bool bLoop, EventSoundEnd callback)
    {
        if (m_pSystem)
        {
            m_pSystem->playSound(m_pSound, nullptr, false, &m_pChannel);
            if (m_pChannel)
            {
                m_pChannel->setMode(bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

                if (callback != nullptr && !bLoop)
                {
                    SoundCallbackInfo info;
                    info.pChannel = m_pChannel;
                    info.callback = callback;

                    SoundManager::GetInstance().AddCallback(info);
                }
            }
        }
    }

    void Sound::Play3D(const XMFLOAT3& position, bool bLoop)
    {
        if (m_pSystem)
        {
            // 소리 재생 시작 (일시정지 상태로)
            m_pSystem->playSound(m_pSound, nullptr, true, &m_pChannel);

            if (m_pChannel)
            {
                // 루프 설정
                m_pChannel->setMode(bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

                // 소리 위치 설정
                FMOD_VECTOR pos = ToFmodVector(position);
                FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
                m_pChannel->set3DAttributes(&pos, &vel);

                // 재생 시작
                m_pChannel->setPaused(false);
            }
        }
    }

    void Sound::Update3DPosition(const XMFLOAT3& position)
    {
        if (m_pChannel)
        {
            bool isPlaying = false;
            m_pChannel->isPlaying(&isPlaying);
            if (isPlaying)
            {
                FMOD_VECTOR pos = ToFmodVector(position);
                FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
                m_pChannel->set3DAttributes(&pos, &vel);
            }
        }
    }

    void Sound::Stop()
    {
        if (m_pChannel) m_pChannel->stop();
    }

    void Sound::SetVolume(float vol)
    {
        if (m_pChannel) m_pChannel->setVolume(vol);
    }


    SoundManager::SoundManager() {}
    SoundManager::~SoundManager() { Release(); }

    void SoundManager::DrawImgui()
    {
        if (ImGui::Button("Refresh List"))
        {
            RefreshSoundList();
        }
        if (ImGui::BeginListBox("##SoundPlayList", ImVec2(-1, 300))) // 높이 300
        {
            for (int i = 0; i < static_cast<int>(m_PlayUIList.size()); ++i)
            {
                bool isSelected = (m_selectedSoundIndex == i);
                std::string label = m_PlayUIList[i];

                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    m_selectedSoundIndex = i;

                    // 클릭 시 바로 로드해서 준비하거나 할 수 있음
                    // LoadSound(m_soundPath + label); 
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    Sound* snd = LoadSound(label, false);

                    // event
                    if (snd) snd->Play2D(false, []() {
                        //std::cout << "play 끝!" << std::endl;
                    });
                }

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("SOUND_ORDER", &i, sizeof(int));
                    ImGui::Text(label.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SOUND_ORDER"))
                    {
                        int fromIdx = *(int*)payload->Data;
                        int toIdx = i;
                        if (fromIdx != toIdx)
                        {
                            std::string temp = m_PlayUIList[fromIdx];
                            m_PlayUIList.erase(m_PlayUIList.begin() + fromIdx);
                            m_PlayUIList.insert(m_PlayUIList.begin() + toIdx, temp);
                            m_selectedSoundIndex = toIdx;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove from List"))
                    {
                        m_PlayUIList.erase(m_PlayUIList.begin() + i);
                        ImGui::EndPopup();
                        break;
                    }
                    /*/ 파일 삭제 기능
                    if (ImGui::MenuItem("Delete File (Warning!)"))
                    {
                        fs::remove(m_soundPath + label);
                        m_playList.erase(m_playList.begin() + i);
                        ImGui::EndPopup();
                        break;
                    }
                    */
                    ImGui::EndPopup();
                }
            }
            ImGui::EndListBox();
        }
    }

    bool SoundManager::Init()
    {
        FMOD_RESULT ret;
        ret = FMOD::System_Create(&m_pSystem);
        if (ret != FMOD_OK) return false;

        // 512 채널, 일반 초기화
        ret = m_pSystem->init(512, FMOD_INIT_NORMAL, nullptr);
        if (ret != FMOD_OK) return false;

        // 3D 세팅: 거리 계수 (1.0f = 미터 단위)
        m_pSystem->set3DSettings(1.0f, 1.0f, 1.0f);

        return true;
    }

    void SoundManager::Update(const XMFLOAT3& camPos, const XMFLOAT3& camForward, const XMFLOAT3& camUp)
    {
        if (!m_pSystem) return;

        // 카메라 위치 업데이트
        FMOD_VECTOR pos = ToFmodVector(camPos);
        FMOD_VECTOR forward = ToFmodVector(camForward);
        FMOD_VECTOR up = ToFmodVector(camUp);
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

        for (auto iter = m_callbackList.begin(); iter != m_callbackList.end(); )
        {
            bool isPlaying = false;
            FMOD::Channel* ch = iter->pChannel;

            if (ch)
            {
                ch->isPlaying(&isPlaying);
            }

            if (!isPlaying)
            {
                if (iter->callback)
                {
                    iter->callback();
                }

                iter = m_callbackList.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        // 리스너 0번 업데이트
        m_pSystem->set3DListenerAttributes(0, &pos, &vel, &forward, &up);

        if (m_pSystem) m_pSystem->update();
    }

    Sound* SoundManager::LoadSound(std::string filename, bool is3D)
    {
        if (m_SoundList.find(filename) != m_SoundList.end())
            return m_SoundList[filename];

        Sound* sound = new Sound(m_pSystem, m_index++, filename);

        FMOD_MODE mode = FMOD_DEFAULT;
        if (is3D)
        {
            mode = FMOD_3D | FMOD_3D_LINEARROLLOFF;
        }
        else
        {
            mode = FMOD_2D;
        }

        FMOD_RESULT ret = m_pSystem->createSound((m_soundPath + filename).c_str(), mode, 0, &sound->m_pSound);

        if (ret != FMOD_OK)
        {
            delete sound;
            return nullptr;
        }

        if (is3D)
        {
            sound->m_pSound->set3DMinMaxDistance(1.0f, 100.0f);
        }

        m_SoundList.insert(make_pair(filename, sound));
        return sound;
    }

    Sound* SoundManager::GetSound(std::string filename)
    {
        auto iter = m_SoundList.find(filename);

        if (iter != m_SoundList.end())
        {
            return iter->second;
        }

        return nullptr;
    }

    void SoundManager::RefreshSoundList()
    {
        m_PlayUIList.clear();

        if (fs::exists(m_soundPath) && fs::is_directory(m_soundPath))
        {
            for (const auto& entry : fs::directory_iterator(m_soundPath))
            {
                if (entry.is_regular_file())
                {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
                    {
                        m_PlayUIList.push_back(entry.path().filename().string());
                    }
                }
            }
        }
    }

    bool SoundManager::Release()
    {
        m_callbackList.clear();

        for (auto& pair : m_SoundList)
        {
            pair.second->Release();
            delete pair.second;
        }
        m_SoundList.clear();

        if (m_pSystem)
        {
            m_pSystem->close();
            m_pSystem->release();
            m_pSystem = nullptr;
        }

        return true;
    }


}