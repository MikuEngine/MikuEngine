#include "EnginePCH.h"
#include "AudioSource.h"

#include "Common/Utility/StaticMemoryPool.h"
#include "Framework/System/SoundSystem.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/SoundData.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Editor/EditorManager.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/Object/Component/Camera.h"
#include "Common/Utility/EditorHelper.h"
#include "fmod.hpp"


// "Audio Files\0*.wav;*.mp3;*.ogg;*.flac\0All Files\0*.*\0";

namespace engine
{
    namespace
    {
        StaticMemoryPool<AudioSource, 256> g_audioSourcePool;
    }

    void* AudioSource::operator new(size_t size)
    {
        return g_audioSourcePool.Allocate(size);
    }

    void AudioSource::operator delete(void* ptr)
    {
        g_audioSourcePool.Deallocate(ptr);
    }

    AudioSource::AudioSource()
    {
    }

    AudioSource::~AudioSource()
    {   
        Stop();
        SoundSystem::Get().Unregister(this);
    }

    void AudioSource::Initialize()
    {
        SoundSystem::Get().Register(this);

        if (!m_clipName.empty())
        {
            SetClip(m_clipName);

            if (m_playOnAwake && EditorManager::Get().GetEditorState() != EditorState::Edit)
            {
                Play();
            }
        }
    }

    void AudioSource::OnDestroy()
    {
        Stop();
    }

    void AudioSource::Update()
    {
        if (m_fadeState != FadeState::None)
        {
            if (!m_currentChannel)
            {
                m_fadeState = FadeState::None;
                return;
            }

            m_fadeTimer += Time::DeltaTime();
            float t = 0.0f;
            if (m_fadeDuration > 0.0f)
            {
                t = std::clamp(m_fadeTimer / m_fadeDuration, 0.0f, 1.0f);
            }
            else
            {
                t = 1.0f;
            }

            float currentVol = std::lerp(m_startVolume, m_targetVolume, t);
            m_currentChannel->setVolume(currentVol);

            if (t >= 1.0f)
            {
                if (m_fadeState == FadeState::FadingOut)
                {
                    Stop();
                }
                
                m_fadeState = FadeState::None;
            }
        }

        if (m_isPlaying && m_isAutoStop && m_fadeState == FadeState::None)
        {
            m_sustainTimer += Time::DeltaTime();
            if (m_sustainTimer >= m_sustainDuration)
            {
                Stop(m_scheduledFadeOutDuration);
                m_isAutoStop = false;
            }
        }
    }

    void AudioSource::SetClip(std::string name)
    {
        Stop();
        m_clipName = name;

        m_soundData = AssetManager::Get().GetOrCreateSoundData(name, m_bus);
    }

    void AudioSource::Play(EventCallBack callback, float fadeInDuration)
    {
        m_isAutoStop = false;

        std::string clipToPlay = m_clipName;
        
        if (m_useRandom && !m_randomClipNames.empty())
        {
            int randomIndex = 0;

            if (m_randomClipNames.size() > 1)
            {
                int loopCount = 0;
                do
                {
                    randomIndex = rand() % m_randomClipNames.size();
                    loopCount++;
                } while (randomIndex == m_lastRandomIndex && loopCount < 10);
            }

            m_lastRandomIndex = randomIndex;
            clipToPlay = m_randomClipNames[randomIndex];
        }

        auto soundData = AssetManager::Get().GetOrCreateSoundData(clipToPlay, m_bus);
        if (!soundData) return;
        m_soundData = soundData;
        Sound* soundResource = soundData->GetSound();
        if (!soundResource) return;

        Stop();

        if (m_is3D)
        {
            Transform* tr = GetTransform();
            if (tr)
            {
                m_currentChannel = soundResource->Play3D(tr->GetWorldPosition(), m_isLoop);
                m_currentChannel->setMode(FMOD_3D_LINEARROLLOFF);
                m_isPlaying = true;
            }
        }
        else
        {
            m_currentChannel = soundResource->Play2D(m_isLoop, callback);
            m_isPlaying = true;
        }

        if (m_currentChannel)
        {
            FMOD::ChannelGroup* cg = nullptr;
            m_currentChannel->getChannelGroup(&cg);
            if (cg)
            {
                char n[128] = {};
                cg->getName(n, 128);
            }

            // 그룹 할당
            if (soundResource->m_pChannelGroup)
            {
                m_currentChannel->setChannelGroup(soundResource->m_pChannelGroup);
            }

            if (m_currentChannel)
            {
                FMOD::ChannelGroup* cg = nullptr;
                m_currentChannel->getChannelGroup(&cg);
                if (cg)
                {
                    char n[128] = {};
                    cg->getName(n, 128);
                }
            }

            if (m_is3D)
            {
                m_currentChannel->set3DMinMaxDistance(m_minDistance, m_maxDistance);
            }

            if (m_useRandom)
            {
                float randomPitch = 0.95f + (static_cast<float>(rand()) / RAND_MAX) * 0.1f;
                m_currentChannel->setPitch(randomPitch);
            }
            else
            {
                m_currentChannel->setPitch(1.0f);
            }

			// fade in 처리
            if (fadeInDuration > 0.0f)
            {
                m_fadeState = FadeState::FadingIn;
                m_fadeDuration = fadeInDuration;
                m_fadeTimer = 0.0f;
                m_startVolume = 0.0f;
                m_targetVolume = m_volume;
                m_currentChannel->setVolume(0.0f);
            }
            else
            {
                m_fadeState = FadeState::None;
                m_currentChannel->setVolume(m_volume);
            }
        }
    }

    void AudioSource::Play(float fadeIn, float duration, float fadeOut)
    {
        Play(nullptr, fadeIn);
        
        if (m_isPlaying)
        {
            m_isAutoStop = true;
            m_sustainTimer = 0.0f;
            m_sustainDuration = duration;
            m_scheduledFadeOutDuration = fadeOut;
        }
    }

    void AudioSource::Stop(float fadeOutDuration)
    {
        if (m_currentChannel)
        {
            bool isPlaying = false;
            m_currentChannel->isPlaying(&isPlaying);
            
            if (isPlaying)
            {
                if (fadeOutDuration > 0.0f)
                {
                    m_isAutoStop = false;

                    float currentVol = 0.0f;
                    m_currentChannel->getVolume(&currentVol);

                    m_fadeState = FadeState::FadingOut;
                    m_fadeDuration = fadeOutDuration;
                    m_fadeTimer = 0.0f;
                    m_startVolume = currentVol;
                    m_targetVolume = 0.0f;
                }
                else
                {
                    Sound* soundResource = GetSoundResource();
                    if (soundResource)
                    {
                        m_currentChannel->stop();
                    }
                    m_currentChannel = nullptr;
                    m_isPlaying = false;
                    m_fadeState = FadeState::None;
                    m_isAutoStop = false;
                }
            }
            else
            {
                m_currentChannel = nullptr;
                m_isPlaying = false;
                m_fadeState = FadeState::None;
                m_isAutoStop = false;
            }
        }
        else
        {
            m_currentChannel = nullptr;
            m_isPlaying = false;
            m_fadeState = FadeState::None;
            m_isAutoStop = false;
        }
    }

    void AudioSource::LoadClipFromFile()
    {
        std::string selectedPath = OpenAudioFileDialog("Audio Files\0*.wav;*.mp3;*.ogg;*.flac\0All Files\0*.*\0");

        if (selectedPath.empty()) return;

        fs::path sourcePath(selectedPath);
        fs::path destDir = SoundSystem::Get().GetSoundPath();
        fs::path destPath = destDir / sourcePath.filename();

        try
        {
            if (!fs::exists(destDir))
            {
                fs::create_directories(destDir);
            }

            std::error_code ec;
            bool isSameFile = false;

            if (fs::exists(destPath) && fs::equivalent(sourcePath, destPath, ec))
            {
                isSameFile = true;
            }
            if(!isSameFile)
            {
                fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
            }

            m_clipName = sourcePath.filename().string();
            SetClip(m_clipName);

            SoundSystem::Get().RefreshSoundList();
        }
        catch (fs::filesystem_error& e)
        {
            LOG_ERROR("AudioSource :: File Copy Failed! {}", e.what());
        }
    }

    void AudioSource::SetVolume(float vol)
    {
        m_volume = vol;
        Sound* soundResource = GetSoundResource();
        if (soundResource)
        {
            soundResource->SetVolume(vol);
        }
    }

    void AudioSource::SetLoop(bool loop)
    {
        m_isLoop = loop;
    }

    void AudioSource::Set3D(bool enable)
    {
        if (m_is3D != enable)
        {
            bool wasPlaying = m_isPlaying;

            if (m_fadeState != FadeState::None) wasPlaying = false;

            m_is3D = enable;

            if (!m_clipName.empty())
            {
                SetClip(m_clipName);
            }

            if (wasPlaying)
            {
                Play();
            }
        }
    }

    void AudioSource::SetForceStopState()
    {
        m_isPlaying = false;
        m_currentChannel = nullptr;
    }

    void AudioSource::OnSoundEnd()
    {
        m_isPlaying = false;
    }

    void AudioSource::OnGui()
    {
        if (ImGui::TreeNodeEx("Audio Source", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char buffer[256];
            strcpy_s(buffer, m_clipName.c_str());

            EditorState currentState = EditorManager::Get().GetEditorState();

            ImGui::Spacing();

            const char* items[] = { "SFX", "BGM", "UI" };
            int item_current = 0;
            if (m_bus == "BGM") item_current = 1;
            else if (m_bus == "UI") item_current = 2;

            if (ImGui::Combo("Sound Type", &item_current, items, IM_ARRAYSIZE(items)))
            {
                if (item_current == 0) m_bus = "SFX";
                else if (item_current == 1) m_bus = "BGM";
                else if (item_current == 2) m_bus = "UI";

                if (!m_clipName.empty()) SetClip(m_clipName);
            }

            ImGui::Checkbox("Use Random Clips", &m_useRandom);

            if (m_useRandom)
            {
                ImGui::Indent(10.0f);

                for (int i = 0; i < m_randomClipNames.size(); ++i)
                {
                    ImGui::PushID(i);

                    char buffer[256];
                    strcpy_s(buffer, m_randomClipNames[i].c_str());

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
                    if (ImGui::InputText("##Clip", buffer, sizeof(buffer)))
                    {
                        m_randomClipNames[i] = buffer;
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("X"))
                    {
                        RemoveRandomClip(i);
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }

                if (ImGui::Button("+ Add Clip", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    std::vector<std::string> selectedPaths = engine::OpenAudioFilesDialog("Audio Files\0*.wav;*.mp3;*.ogg;*.flac\0All Files\0*.*\0");

                    if (!selectedPaths.empty())
                    {
                        for (const std::string& pathStr : selectedPaths)
                        {
                            fs::path sourcePath(pathStr);
                            fs::path destDir = SoundSystem::Get().GetSoundPath();
                            fs::path destPath = destDir / sourcePath.filename();

                            try
                            {
                                if (!fs::exists(destDir)) fs::create_directories(destDir);

                                if (!fs::exists(destPath) || !fs::equivalent(sourcePath, destPath))
                                {
                                    fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
                                }

                                AddRandomClip(sourcePath.filename().string());
                            }
                            catch (const std::exception& e)
                            {
                                LOG_ERROR("Clip Import Failed: {}", e.what());
                            }
                        }
                    }
                }

                ImGui::Unindent(10.0f);
            }
            else
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
                if (ImGui::InputText("##Clip Name", buffer, sizeof(buffer)))
                {
                    m_clipName = buffer;
                }
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    SetClip(m_clipName);
                    Play();
                }


                ImGui::BeginDisabled(currentState == EditorState::Play);
                ImGui::SameLine();
                if (ImGui::Button("Load"))
                {
                    LoadClipFromFile();
                }
                ImGui::EndDisabled();

            }

            if (currentState == EditorState::Edit)
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Preview Control]");

                ImGui::BeginDisabled(m_isPlaying);
                if (ImGui::Button("Play"))
                {
                    Play(nullptr, m_fadeInTime);
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(!m_isPlaying);
                if (ImGui::Button("Stop"))
                {
                    Stop(m_fadeOutTime);
                }
                ImGui::EndDisabled();
                ImGui::Separator();
            }

            bool loop = m_isLoop;
            if (ImGui::Checkbox("Loop", &loop))
            {
                SetLoop(loop);
                if (loop && currentState == EditorState::Play)
                {
                    Play();
                }
                else
                {
                    Stop();
                }
            }

            ImGui::Checkbox("is3D", &m_is3D);

            if (m_is3D)
            {
                GameObject* listener = SoundSystem::Get().GetListenerTarget();
                if (listener)
                {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Listener: Target [%s]", listener->GetName().c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Listener: Main Camera (Default)");
                }

                Vector3 listenerPos = Vector3::Zero;
                if (listener)
                {
                    listenerPos = listener->GetTransform()->GetWorldPosition();
                }
                else
                {
                    auto mainCam = SystemManager::Get().GetCameraSystem().GetMainCamera();
                    if (mainCam)
                    {
                        listenerPos = mainCam->GetTransform()->GetWorldPosition();
                    }
                }

                Transform* myTr = GetTransform();
                if (myTr)
                {
                    float dist = Vector3::Distance(listenerPos, myTr->GetWorldPosition());

                    if (dist > m_maxDistance)
                    {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Distance: %.1f (Muted)", dist);
                    }
                    else
                    {
                        ImGui::Text("Distance: %.1f", dist);
                    }
                }

                ImGui::Spacing();
                ImGui::Indent(10.0f);

                ImGui::DragFloat("Min Distance", &m_minDistance);
                ImGui::DragFloat("Max Distance", &m_maxDistance);

                ImGui::Unindent(10.0f);
            }

            ImGui::BeginDisabled(currentState == EditorState::Play);
            ImGui::Checkbox("Play On Awake", &m_playOnAwake);
            ImGui::EndDisabled();
            

            ImGui::Spacing();
            ImGui::Indent(10.0f);

            float vol = m_volume;
            if (ImGui::DragFloat("Volume", &vol, 0.01f, 0.0f, 1.0f, "%.2f"))
            {
                SetVolume(vol);
            }

            ImGui::DragFloat("Fade In (sec)", &m_fadeInTime, 0.1f, 0.0f, 60.0f, "%.1f");
            ImGui::DragFloat("Fade Out (sec)", &m_fadeOutTime, 0.1f, 0.0f, 60.0f, "%.1f");

            ImGui::Unindent(10.0f);

            ImGui::TreePop();
        }
    }

    void AudioSource::Save(json& j) const
    {
        Object::Save(j);

        j["ClipName"] = m_clipName;
        j["IsLoop"] = m_isLoop;
        j["Is3D"] = m_is3D;
        j["PlayOnAwake"] = m_playOnAwake;
        j["Volume"] = m_volume;
        j["MinDist"] = m_minDistance;
        j["MaxDist"] = m_maxDistance;
        j["UseRandom"] = m_useRandom;
        j["RandomClips"] = m_randomClipNames;
        j["Bus"] = m_bus;
    }

    void AudioSource::Load(const json& j)
    {
        Object::Load(j);

        if (j.contains("ClipName")) m_clipName = j["ClipName"];
        if (j.contains("IsLoop")) m_isLoop = j["IsLoop"];
        if (j.contains("Is3D")) m_is3D = j["Is3D"];
        if (j.contains("PlayOnAwake")) m_playOnAwake = j["PlayOnAwake"];
        if (j.contains("Volume")) m_volume = j["Volume"];
        if (j.contains("MinDist")) m_minDistance = j["MinDist"];
        if (j.contains("MaxDist")) m_maxDistance = j["MaxDist"];
        if (j.contains("UseRandom")) m_useRandom = j["UseRandom"];
        if (j.contains("RandomClips"))
        {
            m_randomClipNames = j["RandomClips"].get<std::vector<std::string>>();
        }
        if (j.contains("Bus")) m_bus = j["Bus"];
    }
    
    Sound* AudioSource::GetSoundResource() const
    {
        return m_soundData ? m_soundData->GetSound() : nullptr;
    }
}
