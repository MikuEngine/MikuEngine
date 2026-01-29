#include "EnginePCH.h"
#include "SoundSystem.h"

#include <fstream>
#include <filesystem>

#include "Framework/Asset/SoundData.h"
// #include "Common/Utility/StringHelper.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Asset/AssetManager.h"
#include "Core/System/VirtualFileSystem.h"
#include "Framework/Object/Component/Renderer/DebugRenderer.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/Object/Component/Camera.h"

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

    void SoundSystem::ApplyVolumes()
    {
        if (!m_pSystem) return;

        const float master = m_mute ? 0.0f : Clamp01(m_master);

        if (auto* g = GetOrCreateChannelGroup("Master"))
            g->setVolume(master);

        if (auto* g = GetOrCreateChannelGroup("BGM"))
            g->setVolume(Clamp01(m_bgm));

        if (auto* g = GetOrCreateChannelGroup("SFX"))
            g->setVolume(Clamp01(m_sfx));
    }

    float SoundSystem::Clamp01(float v)
    {
        if (v < 0.f) return 0.f;
        if (v > 1.f) return 1.f;
        return v;
    }

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

        FMOD::ChannelGroup* masterGroup = nullptr;
        m_pSystem->getMasterChannelGroup(&masterGroup);

        if (masterGroup)
        {
            m_channelGroups["Master"] = masterGroup;

            // 기본 그룹들 생성/확보
            GetOrCreateChannelGroup("BGM");
            GetOrCreateChannelGroup("SFX");
            GetOrCreateChannelGroup("UI");
        }

        ApplyVolumes();
        return true;
    }

    void SoundSystem::Shutdown()
    {
        m_SoundQues.clear();
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

        if (source)
        {
            m_registeredAudioSources.insert(source);
        }
    }

    void SoundSystem::Unregister(AudioSource* source)
    {
        if (source)
        {
            m_registeredAudioSources.erase(source);
        }
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

    void SoundSystem::Render()
    {
        if (!m_showDebugRanges) return;

        for (AudioSource* source : m_registeredAudioSources)
        {
            if (!source || !source->IsActive() || !source->Is3D())
                continue;

            Transform* tr = source->GetTransform();
            if (!tr) continue;

            Vector3 center = tr->GetWorldPosition();
            float minR = source->GetMinDistance();
            float maxR = source->GetMaxDistance();

            auto Draw3DSphere = [&](const Vector3& pos, float radius, const DirectX::XMVECTOR& color)
                {
                    DebugRenderer::Get().DrawRing(pos, radius, Vector3::Up, color);
                    DebugRenderer::Get().DrawRing(pos, radius, Vector3::Forward, color);
                    DebugRenderer::Get().DrawRing(pos, radius, Vector3::Right, color);

                    Vector3 diag1 = Vector3(1.0f, 0.0f, 1.0f);
                    diag1.Normalize();
                    DebugRenderer::Get().DrawRing(pos, radius, diag1, color);

                    Vector3 diag2 = Vector3(1.0f, 0.0f, -1.0f);
                    diag2.Normalize();
                    DebugRenderer::Get().DrawRing(pos, radius, diag2, color);
                };

            Draw3DSphere(center, std::max(0.1f, minR), DirectX::XMVectorSet(0.5f, 0.5f, 0.5f, 0.8f));
            Draw3DSphere(center, std::max(0.1f, maxR), DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 0.8f));
        }
    }

    void SoundSystem::Update()
    {
        if (!m_pSystem)
            return;

        bool listenerUpdated = false;

        if (m_listenerTarget && m_listenerTarget->IsActive())
        {
            Transform* tr = m_listenerTarget->GetTransform();
            if (tr)
            {
                m_listenerPos = tr->GetWorldPosition();
                listenerUpdated = true;
            }
        }

        if (!listenerUpdated)
        {
            auto& cameraSystem = SystemManager::Get().GetCameraSystem();
            auto mainCam = cameraSystem.GetMainCamera();
            if (mainCam)
            {
                Transform* tr = mainCam->GetTransform();
                if (tr)
                {
                    m_listenerPos = tr->GetWorldPosition();
                }
            }
        }

        // Up, Forward vector 쿼터뷰라 고정
        m_listenerForward = Vector3(0.0f, 0.0f, 1.0f);
        m_listenerUp = Vector3(0.0f, 1.0f, 0.0f);

        if (m_listenerForward.LengthSquared() <= FLT_EPSILON)
        {
            m_listenerForward = Vector3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            m_listenerForward.Normalize();
        }

        if (m_listenerUp.LengthSquared() <= FLT_EPSILON)
        {
            m_listenerUp = Vector3(0.0f, 1.0f, 0.0f);
        }
        else
        {
            m_listenerUp.Normalize();
        }

        float dot = m_listenerForward.Dot(m_listenerUp);
        if (abs(dot) > 0.99f)
        {
            m_listenerUp = Vector3::UnitX.Cross(m_listenerForward);
            if (m_listenerUp.LengthSquared() <= FLT_EPSILON)
                m_listenerUp = Vector3::UnitZ.Cross(m_listenerForward);
            m_listenerUp.Normalize();
        }

        FMOD_VECTOR pos = ToFmodVector(m_listenerPos);
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        FMOD_VECTOR forward = ToFmodVector(m_listenerForward);
        FMOD_VECTOR up = ToFmodVector(m_listenerUp);

        // 리스너 0번 업데이트 (메인 카메라)
        m_pSystem->set3DListenerAttributes(0, &pos, &vel, &forward, &up);


        // 등록된 AudioSource 컴포넌트들의 위치 업데이트
        for (AudioSource* source : m_components)
        {
            source->Update();

            if (source->IsActive() && source->Is3D() && source->IsPlaying())
            {
                FMOD::Channel* channel = source->GetChannel();
                Transform* transform = source->GetGameObject()->GetTransform();

                if (channel && transform)
                {
                    FMOD_VECTOR sourcePos = ToFmodVector(transform->GetWorldPosition());
                    FMOD_VECTOR sourceVel = { 0.0f, 0.0f, 0.0f };

                    channel->set3DAttributes(&sourcePos, &sourceVel);
                }
            }

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

                if (res == FMOD_ERR_INVALID_HANDLE)
                {
                    source->SetForceStopState();
                }
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
        if (!m_pSystem) return nullptr;

        if (groupName == "Master")
        {
            FMOD::ChannelGroup* masterGroup = nullptr;
            m_pSystem->getMasterChannelGroup(&masterGroup);

            if (masterGroup)
            {
                m_channelGroups["Master"] = masterGroup;
            }
            return masterGroup;
        }

        if (m_channelGroups.contains(groupName))
        {
            return m_channelGroups[groupName];
        }

        FMOD::ChannelGroup* parentGroup = GetOrCreateChannelGroup("Master");
        if (!parentGroup) return nullptr;

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
            if (!currentPath.empty()) currentPath += "/";
            currentPath += remainingPath;

            if (!m_channelGroups.contains(currentPath))
            {
                FMOD::ChannelGroup* newGroup = nullptr;
                m_pSystem->createChannelGroup(remainingPath.c_str(), &newGroup);
                parentGroup->addGroup(newGroup);
                m_channelGroups[currentPath] = newGroup;
            }

            return m_channelGroups[currentPath];
        }

        return parentGroup;
    }

    Sound* SoundSystem::CreateSound(const std::string &filename, const std::string &option)
    {
        bool isBGM = (option.find("BGM") != std::string::npos);
        bool is3D = true;
        std::string groupName = option;

        if (isBGM || option.find("UI") != std::string::npos || option == "2D")
        {
            is3D = false;
        }

        FMOD::ChannelGroup* targetGroup = GetOrCreateChannelGroup(groupName);

        // VFS를 통해 파일 로드 시도
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        
        // 경로 정규화
        std::string soundPath = filename;
        if (soundPath.find("Resource/") != 0 && soundPath.find("resource/") != 0)
        {
            soundPath = m_soundPath + filename;
        }
        
        // VFS에서 로드 시도
        bool loaded = vfs.LoadFile(soundPath, fileData);
        
        // VFS에서 실패하면 파일 시스템에서 시도 (개발 모드)
        namespace fs = std::filesystem;
        if (!loaded)
        {
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

            // 파일 시스템에서 로드
            if (fs::exists(targetPath))
            {
                std::ifstream file(targetPath, std::ios::binary | std::ios::ate);
                if (file.is_open())
                {
                    size_t size = static_cast<size_t>(file.tellg());
                    file.seekg(0, std::ios::beg);
                    fileData.resize(size);
                    file.read(reinterpret_cast<char*>(fileData.data()), size);
                    loaded = true;
                }
            }
        }

        if (!loaded || fileData.empty())
        {
            LOG_ERROR("[SoundSystem] File Not Found: {} (Original Input: {})", soundPath, filename);
            return nullptr;
        }

        // 사운드 객체 생성
        Sound *sound = new Sound(m_pSystem, m_index++, filename, targetGroup);

        FMOD_MODE mode = FMOD_DEFAULT;
        if (is3D) mode = FMOD_3D | FMOD_3D_LINEARROLLOFF;
        else      mode = FMOD_2D;

        // FMOD 메모리 버퍼에서 로드
        FMOD_CREATESOUNDEXINFO exinfo = {};
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = static_cast<unsigned int>(fileData.size());

        // 메모리 버퍼를 FMOD에 전달 (FMOD가 복사하므로 fileData는 해제 가능)
        FMOD_RESULT ret = m_pSystem->createSound(
            reinterpret_cast<const char*>(fileData.data()),
            mode | FMOD_OPENMEMORY,
            &exinfo,
            &sound->m_pSound);

        if (ret != FMOD_OK)
        {
            LOG_ERROR("SoundSystem::CreateSound FAILED / Path: {}", soundPath);
            LOG_ERROR("FMOD Error: {} ({})", FMOD_ErrorString(ret), static_cast<int>(ret));

            delete sound;
            return nullptr;
        }

        return sound;
    }

    void SoundSystem::CreateRandomSound(const std::string& groupName, const std::vector<std::string>& filePaths, const std::string& option, LifeScope scope)
    {
        std::vector<Sound*> soundList;

        for (const auto& path : filePaths)
        {
            auto soundData = AssetManager::Get().GetOrCreateSoundData(path, option, scope);

            if (soundData && soundData->GetSound())
            {
                soundList.push_back(soundData->GetSound());
            }
        }

        if (!soundList.empty())
        {
            m_SoundQues[groupName] = soundList;
        }
    }

    void SoundSystem::SetListenerAttributes(const Vector3& pos, const Vector3& forward, const Vector3& up)
    {
        m_listenerPos = pos;
        m_listenerForward = forward;
        m_listenerUp = up;
    }

    void SoundSystem::Play(const std::string& key, const std::string& option, float volume, float pitch)
    {
        Sound* targetSound = nullptr;

        // random
        auto itRandom = m_SoundQues.find(key);
        if (itRandom != m_SoundQues.end() && !itRandom->second.empty())
        {
            const std::vector<Sound*>& group = itRandom->second;
            int index = rand() % group.size();
            targetSound = group[index];
        }
        else
        {
            auto soundData = AssetManager::Get().GetOrCreateSoundData(key, option, LifeScope::Scene);

            if (soundData)
            {
                targetSound = soundData->GetSound();
            }
        }

        if (!targetSound) return;

        FMOD::Channel* channel = nullptr;

        FMOD::ChannelGroup* targetGroup = targetSound->m_pChannelGroup;
        if (!targetGroup) targetGroup = m_channelGroups["Master"];

        m_pSystem->playSound(targetSound->m_pSound, targetGroup, true, &channel);

        if (channel)
        {
            channel->setMode(FMOD_2D);

            channel->setVolume(volume);
            channel->setPitch(pitch);

            channel->setPaused(false);
        }
    }

    void SoundSystem::SetMasterVolume(float v) { m_master = Clamp01(v); ApplyVolumes(); }
    void SoundSystem::SetBGMVolume(float v) { m_bgm = Clamp01(v); ApplyVolumes(); }
    void SoundSystem::SetSFXVolume(float v) { m_sfx = Clamp01(v); ApplyVolumes(); }
    void SoundSystem::SetMute(bool mute) { m_mute = mute;       ApplyVolumes(); }

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