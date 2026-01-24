#include "EnginePCH.h"
#include "SoundSystem.h"

#include "Common/Utility/StringHelper.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "fmod.hpp"
#include "fmod_errors.h"

namespace engine
{
    // ==============================================================
    // Sound Class Implementation
    // ==============================================================

    Sound::Sound(FMOD::System *system, int index, std::string name, FMOD::ChannelGroup *channelGroup)
        : m_pSystem(system), m_id(index), m_name(name), m_pChannelGroup(channelGroup) {}

    Sound::~Sound()
    {
        Release();
    }

    void Sound::Release()
    {
        if (m_pSound)
        { 
            m_pSound->release(); 
            m_pSound = nullptr;
        }
    }

    FMOD::Channel* Sound::Play2D(bool bLoop, EventCallBack callback)
    {
        if (m_pSystem)
        {
            m_pSystem->playSound(m_pSound, m_pChannelGroup, false, &m_pChannel);
            if (m_pChannel)
            {
                m_pChannel->setMode(bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

                if (callback != nullptr && !bLoop)
                {
                    SoundCallbackInfo info;
                    info.pChannel = m_pChannel;
                    info.callback = callback;

                    SoundSystem::Get().m_callbackList.push_back(info);
                }

                m_pChannel->setPaused(false);
                return m_pChannel;
            }
        }
        return nullptr;
    }

    FMOD::Channel* Sound::Play3D(const Vector3& position, bool bLoop)
    {
        if (m_pSystem)
        {
            m_pSystem->playSound(m_pSound, m_pChannelGroup, true, &m_pChannel);

            if (m_pChannel)
            {
                m_pChannel->setMode(bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

                FMOD_VECTOR pos = ToFmodVector(position);
                FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

                m_pChannel->set3DAttributes(&pos, &vel);
                m_pChannel->set3DMinMaxDistance(1.0f, 500.0f);
                m_pChannel->setPaused(false);

                return m_pChannel;
            }
        }
        return nullptr;
    }

    void Sound::Update3DPosition(const Vector3& position)
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

    // ==============================================================
    // SoundSystem Class Implementation
    // ==============================================================

    bool SoundSystem::Initialize()
    {
        m_components.clear();
        m_callbackList.clear();

        FMOD_RESULT ret;
        ret = FMOD::System_Create(&m_pSystem);
        if (ret != FMOD_OK) return false;

        // 512 채널, 일반 초기화
        ret = m_pSystem->init(512, FMOD_INIT_NORMAL, nullptr);
        if (ret != FMOD_OK) return false;

        // 3D 세팅: 거리 계수 (1.0f = 미터 단위), 왼손 좌표계/오른손 좌표계에 따라 수정 필요
        m_pSystem->set3DSettings(1.0f, 1.0f, 1.0f);

        return true;
    }

    void SoundSystem::Shutdown()
    {
        m_channelGroups.clear();
        m_callbackList.clear();

        if (m_pSystem)
        {
            m_pSystem->close();
            m_pSystem->release();
            m_pSystem = nullptr;
        }
    }

    void SoundSystem::Register(AudioSource* source)
    {
        System<AudioSource>::Register(source);
    }

    void SoundSystem::Unregister(AudioSource* source)
    {
        System<AudioSource>::Unregister(source);
    }

    void SoundSystem::OnGameStart()
    {
        m_components.clear();
        m_callbackList.clear();
    }

    void SoundSystem::StopAll()
    {
        if (m_pSystem)
        {
            FMOD::ChannelGroup* masterGroup = nullptr;
            m_pSystem->getMasterChannelGroup(&masterGroup);

            if (masterGroup)
            {
                masterGroup->stop();
            }
        }
    }

    void SoundSystem::Update()
    {
        if (!m_pSystem)
            return;

        FMOD_VECTOR pos = ToFmodVector(m_listenerPos);
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        FMOD_VECTOR forward = ToFmodVector(m_listenerForward);
        FMOD_VECTOR up = ToFmodVector(m_listenerUp);

        // 리스너 0번 업데이트 (메인 카메라)
        m_pSystem->set3DListenerAttributes(0, &pos, &vel, &forward, &up);

        // 등록된 AudioSource 컴포넌트들의 위치 업데이트
        for (AudioSource* source : m_components)
        {
            // 컴포넌트가 활성화 상태이고, 3D 사운드인 경우에만 위치 갱신
            if (source->IsActive() && source->Is3D())
            {
                Sound* sound = source->GetSoundResource();

                // 현재 재생 중인 사운드가 있다면 위치 동기화
                // (GameObject의 Transform을 가져와서 FMOD 채널에 적용)
                if (sound)
                {
                    Transform* transform = source->GetGameObject()->GetTransform();
                    sound->Update3DPosition(transform->GetWorldPosition());
                }
            }

        source->Update();

        if (source->IsPlaying())
        {
            FMOD::Channel* channel = source->GetChannel();

            if (channel == nullptr)
            {
                source->SetForceStopState();
                continue;
            }

            bool isFmodPlaying = false;
            FMOD_RESULT res = channel->isPlaying(&isFmodPlaying);

            // 채널 소멸 후 핸들 유효하지 않음
            if (res == FMOD_ERR_INVALID_HANDLE)
            {
                source->SetForceStopState();
            }
            // 일시정지거나 막 끝난 직후
            else if (res == FMOD_OK && !isFmodPlaying)
            {
                source->SetForceStopState();
            }
        }
        }

        // 콜백 리스트 처리 (재생 끝난 사운드 콜백 호출)
        for (auto iter = m_callbackList.begin(); iter != m_callbackList.end(); )
        {
            bool isPlaying = false;
            if (iter->pChannel)
            {
                iter->pChannel->isPlaying(&isPlaying);
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

        // FMOD 시스템 틱 업데이트
        m_pSystem->update();
    }

    FMOD::ChannelGroup* SoundSystem::GetOrCreateChannelGroup(const std::string &groupName)
    {
        if (m_channelGroups.contains(groupName))
        {
            return m_channelGroups[groupName];
        }

        FMOD::ChannelGroup *parentGroup = nullptr;
        m_pSystem->getMasterChannelGroup(&parentGroup);

        std::string currentPath;
        std::string remainingPath = groupName;
        size_t pos = 0;

        while ((pos = remainingPath.find('/')) != std::string::npos)
        {
            std::string token = remainingPath.substr(0, pos);
            if (!currentPath.empty())
            {
                currentPath += "/";
            }
            currentPath += token;

            if (!m_channelGroups.contains(currentPath))
            {
                FMOD::ChannelGroup *newGroup = nullptr;
                m_pSystem->createChannelGroup(token.c_str(), &newGroup);
                parentGroup->addGroup(newGroup);
                m_channelGroups[currentPath] = newGroup;
            }

            parentGroup = m_channelGroups[currentPath];
            remainingPath.erase(0, pos + 1);
        }


        if (!remainingPath.empty())
        {
            if (!currentPath.empty())
            {
                currentPath += "/";
            }
        }
        currentPath += remainingPath;

        if (!m_channelGroups.contains(currentPath))
        {
            FMOD::ChannelGroup *newGroup = nullptr;
            m_pSystem->createChannelGroup(remainingPath.c_str(), &newGroup);
            parentGroup->addGroup(newGroup);
            m_channelGroups[currentPath] = newGroup;
        }
        parentGroup = m_channelGroups[currentPath];
        
        return parentGroup;
    }

    Sound* SoundSystem::CreateSound(const std::string &filename, const std::string &option)
    {
        bool is3D = false;
        std::string groupName = option;

        if (option.find("BGM") != std::string::npos)
        {
            is3D = false;
        } 
        else if (option.find("UI") != std::string::npos)
        {
            is3D = false;
        }
        else if (option == "2D")
        {
            is3D = false;
            groupName = "Master";
        }

        FMOD::ChannelGroup* targetGroup = GetOrCreateChannelGroup(groupName);

        namespace fs = std::filesystem;
        fs::path rootPath(m_soundPath);
        fs::path inputPath(filename);
        fs::path targetPath;

        std::string inputStr = inputPath.generic_string();
        std::string rootStr = rootPath.generic_string();

        if (fs::exists(inputPath) || inputStr.find(rootStr) == 0)
        {
            targetPath = inputPath;
        }
        else
        {
            targetPath = rootPath / inputPath;
        }

        if (!targetPath.has_extension() || !fs::exists(targetPath))
        {
            fs::path searchDir = targetPath.parent_path();

            if (searchDir.empty() || !fs::exists(searchDir))
            {
                searchDir = rootPath;
            }

            if (fs::exists(searchDir))
            {
                for (const auto& entry : fs::directory_iterator(searchDir))
                {
                    if (entry.is_regular_file())
                    {
                        if (entry.path().stem() == targetPath.stem())
                        {
                            std::string ext = entry.path().extension().string();

                            if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac")
                            {
                                targetPath = entry.path();
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!fs::exists(targetPath))
        {
            LOG_ERROR("[SoundSystem] File Not Found: {} (Original Input: {})", targetPath.string(), filename);
            return nullptr;
        }

        // 사운드 객체 생성
        Sound *sound = new Sound(m_pSystem, m_index++, filename, targetGroup);

        FMOD_MODE mode = FMOD_DEFAULT;
        if (is3D) mode = FMOD_3D | FMOD_3D_LINEARROLLOFF;
        else      mode = FMOD_2D;
		
		//std::string absPathStr = fs::absolute(targetPath).string();
        // Convert path to UTF-8 for proper handling of non-ASCII characters (Korean, etc.)
        std::string absPathStr = PathToUTF8(fs::absolute(targetPath));

        FMOD_RESULT ret = m_pSystem->createSound(absPathStr.c_str(), mode, 0, &sound->m_pSound);

        if (ret != FMOD_OK)
        {
            LOG_ERROR("SoundSystem::CreateSound FAILED / Path: {}", absPathStr);
            LOG_ERROR("FMOD Error: {} ({})", FMOD_ErrorString(ret), static_cast<int>(ret));

            delete sound;
            return nullptr;
        }

        if (is3D)
        {
            // 1미터부터 소리감쇄, 500미터 이후로는 안 들림
            sound->m_pSound->set3DMinMaxDistance(1.0f, 500.0f);
        }

        return sound;
    }

    void SoundSystem::DrawImgui()
    {
        if (ImGui::Button("Refresh List"))
        {
            RefreshSoundList();
        }
        if (ImGui::BeginListBox("##SoundPlayList", ImVec2(-1, 300)))
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
                    //auto soundData = AssetManager::Get().GetOrCreateSoundData(label, LifeScope::Global);

                    /*// EndEvent
                         if (snd) snd->Play2D(false, []()
                         {
                             //std::cout << "play 끝!" << std::endl;
                         });
                     //*/
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
                    //*/
                    ImGui::EndPopup();
                }
            }
            ImGui::EndListBox();
        }
    }

    void SoundSystem::SetListenerAttributes(const Vector3& pos, const Vector3& forward, const Vector3& up)
    {
        m_listenerPos = pos;
        m_listenerForward = forward;
        m_listenerUp = up;
    }

    void SoundSystem::RefreshSoundList()
    {
        namespace fs = std::filesystem;

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
}