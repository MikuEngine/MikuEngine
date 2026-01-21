#include "EnginePCH.h"
#include "AudioSource.h"
#include "Framework/System/SoundSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Editor/EditorManager.h"
#include "Common/Utility/EditorHelper.h"

// "Audio Files\0*.wav;*.mp3;*.ogg;*.flac\0All Files\0*.*\0";

namespace engine
{ 
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

    void AudioSource::SetClip(std::string name)
    {
        Stop();

        m_clipName = name;

        m_soundResource = SoundSystem::Get().GetOrLoadSound(name, m_is3D);
    }

    void AudioSource::Play(EventEndPlay callback)
    {
        if (!m_soundResource) return;

        Stop();

        if (m_is3D)
        {
            Transform* tr = GetTransform();
            if (tr)
            {
                m_currentChannel = m_soundResource->Play3D(tr->GetWorldPosition(), m_isLoop);
                m_isPlaying = true;
            }
        }
        else
        {
            m_currentChannel = m_soundResource->Play2D(m_isLoop, callback);
            m_isPlaying = true;
        }

        if (m_currentChannel)
        {
            m_currentChannel->setVolume(m_volume);
        }
    }

    void AudioSource::Stop()
    {
        if (m_currentChannel)
        {
            bool isPlaying = false;
            m_currentChannel->isPlaying(&isPlaying);
            if (isPlaying)
            {
                if (m_soundResource)
                {
                    m_currentChannel->stop();
                }
            }
        }

        m_currentChannel = nullptr;
        m_isPlaying = false;
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
        if (m_soundResource)
        {
            m_soundResource->SetVolume(vol);
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
            m_is3D = enable;
            if (!m_clipName.empty())
            {
                SetClip(m_clipName);
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

            EditorState currentState = EditorManager::Get().GetEditorState();
            ImGui::BeginDisabled(currentState == EditorState::Play);
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                LoadClipFromFile();
            }
            ImGui::EndDisabled();
            

            if (currentState == EditorState::Edit)
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Preview Control]");

                ImGui::BeginDisabled(m_isPlaying);
                if (ImGui::Button("Play"))
                {
                    Play();
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(!m_isPlaying);
                if (ImGui::Button("Stop"))
                {
                    Stop();
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

            bool is3d = m_is3D;
            if (ImGui::Checkbox("Is 3D", &is3d)) Set3D(is3d);

            ImGui::BeginDisabled(currentState == EditorState::Play);
            ImGui::Checkbox("Play On Awake", &m_playOnAwake);
            ImGui::EndDisabled();
            
            float vol = m_volume;
            if (ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f))
            {
                SetVolume(vol);
            }

            if (m_is3D)
            {
                ImGui::DragFloat("Min Distance", &m_minDistance);
                ImGui::DragFloat("Max Distance", &m_maxDistance);
            }

            ImGui::TreePop();
        }
    }

    void AudioSource::Save(json& j) const
    {
        Component::Save(j);

        j["Type"] = GetType();
        j["ClipName"] = m_clipName;
        j["IsLoop"] = m_isLoop;
        j["Is3D"] = m_is3D;
        j["PlayOnAwake"] = m_playOnAwake;
        j["Volume"] = m_volume;
        j["MinDist"] = m_minDistance;
        j["MaxDist"] = m_maxDistance;
    }

    void AudioSource::Load(const json& j)
    {
        Component::Load(j);

        if (j.contains("ClipName")) m_clipName = j["ClipName"];
        if (j.contains("IsLoop")) m_isLoop = j["IsLoop"];
        if (j.contains("Is3D")) m_is3D = j["Is3D"];
        if (j.contains("PlayOnAwake")) m_playOnAwake = j["PlayOnAwake"];
        if (j.contains("Volume")) m_volume = j["Volume"];
        if (j.contains("MinDist")) m_minDistance = j["MinDist"];
        if (j.contains("MaxDist")) m_maxDistance = j["MaxDist"];
    }
}
