#include "EnginePCH.h"
#include "SkeletalAnimator.h"

#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/AnimationData.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Renderer/SkeletalMeshRenderer.h"

#include <imgui_internal.h>

void to_json(nlohmann::ordered_json& j, engine::AnimationBlendMode mode)
{
    j = nlohmann::ordered_json{ "BlendMode", static_cast<int>(mode) };
}

void from_json(const nlohmann::ordered_json& j, engine::AnimationBlendMode& type)
{
    type = static_cast<engine::AnimationBlendMode>(j.at("BlendMode"));
}

namespace engine
{
    void SkeletalAnimator::Initialize()
    {
        Animator::Initialize();

        InitializeSkeleton();
    }

    void SkeletalAnimator::Awake()
    {
        //InitializeSkeleton();

        if (m_layers.empty())
        {
            AddLayer("Base Layer", 1.0f);
        }
    }

    void SkeletalAnimator::RegisterAnimation(const std::string& name, const std::string& path)
    {
        if (path.empty())
        {
            LOG_ERROR("애니메이션 파일 경로를 지정해주세요");
            return;
        }

        auto animData = AssetManager::Get().GetOrCreateAnimationData(path);
        if (!animData)
        {
            LOG_ERROR("애니메이션 등록 실패: {}", path);
            return;
        }

        AnimationResource res;
        res.data = animData;
        res.path = path;

        m_animations[name] = res;
    }

    void SkeletalAnimator::UnregisterAnimation(const std::string& name)
    {
        m_animations.erase(name);
    }

    void SkeletalAnimator::RenameAnimation(const std::string& oldName, const std::string& newName)
    {
        if (oldName == newName)
        {
            return;
        }

        if (auto iter = m_animations.find(oldName); iter != m_animations.end())
        {
            m_animations[newName] = iter->second;
            m_animations.erase(iter);
        }
    }

    void SkeletalAnimator::AddLayer(const std::string& layerName, float weight)
    {
        m_layers.emplace_back(layerName);
        m_layers.back().weight = weight;

        if (!m_skeleton.empty())
        {
            m_layers.back().mask.assign(m_skeleton.size(), 0);
        }
    }

    void SkeletalAnimator::RemoveLayer(int layerIndex)
    {
        if (layerIndex >= 0 && layerIndex < m_layers.size())
        {
            m_layers.erase(m_layers.begin() + layerIndex);
        }
    }

    void SkeletalAnimator::AddNotify(const std::string& animName, const std::string& notifyName, float time)
    {
        auto it = m_animations.find(animName);
        if (it != m_animations.end())
        {
            AnimationNotify newNotify;
            newNotify.name = notifyName;
            newNotify.time = time;
            it->second.notifies.push_back(newNotify);
        }
    }

    void SkeletalAnimator::BindNotify(const std::string& notifyName, EventCallBack callback)
    {
        m_notifyCallbacks[notifyName] = callback;
    }

    void SkeletalAnimator::UnbindNotify(const std::string& notifyName)
    {
        m_notifyCallbacks.erase(notifyName);
    }

    void SkeletalAnimator::SetLayerMask(int layerIndex, const std::vector<std::string>& boneNames, bool active, bool isRecursive)
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return;
        }

        if (m_skeleton.empty())
        {
            return;
        }

        auto& layer = m_layers[layerIndex];
        layer.mask.assign(m_skeleton.size(), 0);

        for (const auto& name : boneNames)
        {
            for (const auto& bone : m_skeleton)
            {
                if (bone.name == name)
                {
                    layer.mask[bone.index] = active ? 1 : 0;
                    if (isRecursive)
                    {
                        MarkBoneRecursive(layer.mask, bone.index, active);
                    }
                    break;
                }
            }
        }
    }

    void SkeletalAnimator::SetLayerWeight(int layerIndex, float weight)
    {
        if (layerIndex >= 0 && layerIndex < m_layers.size())
        {
            m_layers[layerIndex].weight = std::clamp(weight, 0.0f, 1.0f);
        }
    }

    void SkeletalAnimator::SetLayerBlendMode(int layerIndex, AnimationBlendMode mode)
    {
        if (layerIndex >= 0 && layerIndex < m_layers.size())
        {
            m_layers[layerIndex].blendMode = mode;
        }
    }

    void SkeletalAnimator::SetProceduralRotation(const std::string& boneName, const Quaternion& rotation)
    {
        for (const auto& bone : m_skeleton)
        {
            if (bone.name == boneName)
            {
                m_proceduralRotations[bone.index] = rotation;
                return;
            }
        }
    }

    bool SkeletalAnimator::IsFinished(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return false;
        }

        const auto& layer = m_layers[layerIndex];

        if (layer.next.active)
        {
            return false;
        }

        if (!layer.current.active || !layer.current.data)
        {
            return true; // 재생 중 아니면 끝난 것으로 간주
        }

        if (layer.current.loop)
        {
            return false; // 루프 중이면 절대 끝나지 않음
        }

        float duration = layer.current.data->GetAnimations()[0].duration;

        return layer.current.time >= (duration - 0.001f);
    }
    bool SkeletalAnimator::IsPlaying(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return false;
        }

        return m_layers[layerIndex].current.active;
    }

    bool SkeletalAnimator::IsInTransition(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return false;
        }

        return m_layers[layerIndex].next.active;
    }

    std::string SkeletalAnimator::GetCurrentAnimationName(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return "";
        }

        if (!m_layers[layerIndex].current.active)
        {
            return "";
        }

        return m_layers[layerIndex].current.name;
    }
    float SkeletalAnimator::GetCurrentAnimationTime(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return 0.0f;
        }

        return m_layers[layerIndex].current.time;
    }

    float SkeletalAnimator::GetCurrentAnimationDuration(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return 0.0f;
        }

        if (!m_layers[layerIndex].current.data)
        {
            return 0.0f;
        }

        return m_layers[layerIndex].current.data->GetAnimations()[0].duration;
    }

    float SkeletalAnimator::GetNormalizedTime(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            return 0.0f;
        }

        const auto& layer = m_layers[layerIndex];
        
        if (!layer.current.active || !layer.current.data)
        {
            return 0.0f;
        }

        float duration = layer.current.data->GetAnimations()[0].duration;

        if (duration <= 0.0001f)
        {
            return 0.0f; // 0 나누기 방지
        }

        float currentTime = layer.current.time;
        if (layer.current.loop)
        {
            // 루프 중이면 0.0 ~ 1.0 사이 값으로 순환
            return std::fmod(currentTime, duration) / duration;
        }
        else
        {
            // 루프 아니면 0.0 ~ 1.0 (최대 1.0)
            return std::clamp(currentTime / duration, 0.0f, 1.0f);
        }
    }

    void SkeletalAnimator::Play(const std::string& name, bool loop, int layerIndex, float speed)
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            LOG_ERROR("등록되지 않은 레이어: {}", layerIndex);
            return;
        }

        auto find = m_animations.find(name);
        if (find == m_animations.end())
        {
            LOG_ERROR("등록되지 않은 애니메이션: {}", name);
            return;
        }

        auto& layer = m_layers[layerIndex];

        layer.current.name = name;
        layer.current.data = find->second.data;
        layer.current.time = 0.0f;
        layer.current.speed = speed;
        layer.current.loop = loop;
        layer.current.active = true;

        layer.next.Reset();
        layer.transitionTime = 0.0f;
        layer.transitionDuration = 0.0f;

        if (layerIndex == 0 && !m_skeleton.empty())
        {
            const auto& anims = layer.current.data->GetAnimations();
            if (!anims.empty())
            {
                anims[0].SetupBoneAnimation(m_skeleton);
            }
        }
    }

    void SkeletalAnimator::PlayCrossFade(
        const std::string& name,
        float transitionDuration,
        bool loop,
        int layerIndex,
        float speed)
    {
        if (layerIndex < 0 || layerIndex >= m_layers.size())
        {
            LOG_ERROR("등록되지 않은 레이어: {}", layerIndex);
            return;
        }

        auto find = m_animations.find(name);
        if (find == m_animations.end())
        {
            LOG_ERROR("등록되지 않은 애니메이션: {}", name);
            return;
        }

        auto& layer = m_layers[layerIndex];

        if (!layer.current.active)
        {
            Play(name, loop, layerIndex);
            return;
        }

        if (layer.next.active && layer.next.name == name)
        {
            return;
        }

        if (!layer.next.active && layer.current.name == name)
        {
            return;
        }

        layer.next.name = name;
        layer.next.data = find->second.data;
        layer.next.time = 0.0f;
        layer.next.speed = speed;
        layer.next.loop = loop;
        layer.next.active = true;
        layer.transitionDuration = transitionDuration;
        layer.transitionTime = 0.0f;
    }

    void SkeletalAnimator::Update()
    {
        if (m_skeleton.empty())
        {
            return;
        }

        const float dt = Time::DeltaTime();

        // update layers timeline
        for (auto& layer : m_layers)
        {
            if (!layer.current.active)
            {
                continue;
            }

            float prevTime = layer.current.time; 

            // 1 fbx = 1 anim 이므로 항상 0번 인덱스
            float duration = layer.current.data->GetAnimations()[0].duration;

            layer.current.time += dt * layer.current.speed;

            if (layer.current.time >= duration)
            {
                if (layer.current.loop)
                {
                    layer.current.time = std::fmod(layer.current.time, duration);
                }
                else
                {
                    layer.current.time = duration;
                }
            }

            float currTime = layer.current.time;
            CheckNotifies(layer.current, prevTime, currTime);

            if (layer.next.active)
            {
                layer.transitionTime += dt;

                float nextPrevTime = layer.next.time;
                float nextDuration = layer.next.data->GetAnimations()[0].duration;
                layer.next.time += dt * layer.next.speed;
                if (layer.next.time >= nextDuration)
                {
                    if (layer.next.loop)
                    {
                        layer.next.time += std::fmod(layer.next.time, nextDuration);
                    }
                    else
                    {
                        layer.next.time = nextDuration;
                    }
                }

                float nextCurrTime = layer.next.time;
                CheckNotifies(layer.next, nextPrevTime, nextCurrTime);

                if (layer.transitionTime >= layer.transitionDuration)
                {
                    layer.current = layer.next;
                    layer.next.Reset();
                    layer.transitionTime = 0.0f;

                    if (&layer == &m_layers[0])
                    {
                        layer.current.data->GetAnimations()[0].SetupBoneAnimation(m_skeleton);
                    }
                }
            }
        }

        // evaluate and blend bones
        const auto& boneOffsets = m_skeletonData->GetBoneOffsets();
        
        for (size_t i = 0; i < m_skeleton.size(); ++i)
        {
            auto& bone = m_skeleton[i];

            Vector3 finalPos = bone.local.Translation();
            Quaternion finalRot = Quaternion::CreateFromRotationMatrix(bone.local);
            Vector3 finalScale{ 1.0f, 1.0f, 1.0f };

            for (size_t layerIndex = 0; layerIndex < m_layers.size(); ++layerIndex)
            {
                auto& layer = m_layers[layerIndex];
                if (!layer.current.active)
                {
                    continue;
                }

                if (!layer.mask.empty())
                {
                    if (i >= layer.mask.size() || layer.mask[i] == 0)
                    {
                        continue;
                    }
                }

                // calculate layer pose

                Vector3 curPos;
                Quaternion curRot;
                Vector3 curScale;

                bool hasCur = EvaluateBone(layer.current, bone.name, layer.current.time, curPos, curRot, curScale);

                if (!hasCur)
                {
                    continue;
                }

                Vector3 layerPos = curPos;
                Quaternion layerRot = curRot;
                Vector3 layerScale = curScale;

                if (layer.next.active)
                {
                    Vector3 nextPos;
                    Quaternion nextRot;
                    Vector3 nextScale;

                    bool hasNext = EvaluateBone(layer.next, bone.name, layer.next.time, nextPos, nextRot, nextScale);

                    if (hasNext)
                    {
                        float t = std::clamp(layer.transitionTime / layer.transitionDuration, 0.0f, 1.0f);

                        layerPos = Vector3::Lerp(layerPos, nextPos, t);
                        layerRot = Quaternion::Slerp(layerRot, nextRot, t);
                        layerScale = Vector3::Lerp(layerScale, nextScale, t);
                    }
                }

                // apply layer weight
                float w = layer.weight;

                if (layer.blendMode == AnimationBlendMode::Additive)
                {
                    Vector3 refPos;
                    Quaternion refRot;
                    Vector3 refScale;

                    EvaluateBone(layer.current, bone.name, 0.0f, refPos, refRot, refScale);

                    Vector3 deltaPos = layerPos - refPos;
                    Quaternion inv;
                    refRot.Inverse(inv);
                    Quaternion deltaRot = layerRot * inv;
                    
                    finalPos += deltaPos * w;
                    finalRot = finalRot * Quaternion::Slerp(Quaternion::Identity, deltaRot, w);
                }
                else // override
                {
                    if (layerIndex == 0)
                    {
                        finalPos = layerPos;
                        finalRot = layerRot;
                        finalScale = layerScale;
                    }
                    else
                    {
                        finalPos = Vector3::Lerp(finalPos, layerPos, w);
                        finalRot = Quaternion::Slerp(finalRot, layerRot, w);
                        finalScale = Vector3::Lerp(finalScale, layerScale, w);
                    }
                }
            }

            // procedural rotation
            if (auto iter = m_proceduralRotations.find(static_cast<int>(i));
                iter != m_proceduralRotations.end())
            {
                finalRot = finalRot * iter->second;
            }

            Matrix nodeTrans =
                Matrix::CreateScale(finalScale) *
                Matrix::CreateFromQuaternion(finalRot) *
                Matrix::CreateTranslation(finalPos);

            if (bone.parentIndex != -1)
            {
                bone.model = nodeTrans * m_skeleton[bone.parentIndex].model;
            }
            else
            {
                bone.model = nodeTrans;
            }

            m_finalBoneMatrices[bone.index] = (boneOffsets[bone.index] * bone.model).Transpose();
        }
    }

    void SkeletalAnimator::DrawTimeline(const std::string& animName)
    {
        auto iter = m_animations.find(animName);
        if (iter == m_animations.end()) return;

        auto& res = iter->second;
        auto& notifies = res.notifies;

        if (!res.data || res.data->GetAnimations().empty()) return;
        float duration = res.data->GetAnimations()[0].duration;

        if (m_layers.empty()) return;

        int targetLayerIndex = 0;
        bool isFoundPlayingLayer = false;

        for (int i = 0; i < m_layers.size(); ++i)
        {
            if (m_layers[i].current.active && m_layers[i].current.name == animName)
            {
                targetLayerIndex = i;
                isFoundPlayingLayer = true;
                break;
            }

            if (m_layers[i].next.active && m_layers[i].next.name == animName)
            {
                targetLayerIndex = i;
                isFoundPlayingLayer = true;
                break;
            }
        }

        auto& layer = m_layers[targetLayerIndex];

        float displayTime = 0.0f;
        if (isFoundPlayingLayer)
        {
            if (layer.next.active && layer.next.name == animName)
            {
                displayTime = layer.next.time;
            }
            else if (layer.current.active && layer.current.name == animName)
            {
                displayTime = layer.current.time;
            }
        }
        else
        {
            displayTime = 0.0f;
        }

        ImGui::Text("Timeline (Duration: %.2fs)", duration);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 50.0f;

        drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + width, cursorPos.y + height), IM_COL32(40, 40, 40, 255));
        drawList->AddRect(cursorPos, ImVec2(cursorPos.x + width, cursorPos.y + height), IM_COL32(100, 100, 100, 255));

        ImGui::InvisibleButton("##TimelineArea", ImVec2(width, height));

        bool isHovered = ImGui::IsItemHovered();
        bool isDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool isRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

        for (int i = 0; i <= 10; ++i)
        {
            float ratio = (float)i / 10.0f;
            float x = cursorPos.x + width * ratio;
            drawList->AddLine(ImVec2(x, cursorPos.y), ImVec2(x, cursorPos.y + height), IM_COL32(80, 80, 80, 255));
        }

        int hoveredNotifyIndex = -1;

        for (int i = 0; i < notifies.size(); ++i)
        {
            auto& notify = notifies[i];
            float ratio = notify.time / duration;
            float x = cursorPos.x + width * ratio;
            float y = cursorPos.y + height * 0.5f;
            float size = 6.0f;

            ImVec2 p1(x, y - size); ImVec2 p2(x + size, y);
            ImVec2 p3(x, y + size); ImVec2 p4(x - size, y);

            bool isMarkerHovered = false;
            ImVec2 mousePos = ImGui::GetMousePos();
            if (mousePos.x >= x - size && mousePos.x <= x + size &&
                mousePos.y >= y - size && mousePos.y <= y + size)
            {
                isMarkerHovered = true;
                hoveredNotifyIndex = i;
            }

            if (isMarkerHovered)
            {
                drawList->AddQuadFilled(p1, p2, p3, p4, IM_COL32(100, 255, 100, 255));
                ImGui::SetTooltip("%s (%.2f)", notify.name.c_str(), notify.time);

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    ImGui::OpenPopup(("DeleteNotifyPopup_" + std::to_string(i)).c_str());
                }
            }
            else
            {
                drawList->AddQuadFilled(p1, p2, p3, p4, IM_COL32(0, 255, 0, 255));
            }

            drawList->AddQuad(p1, p2, p3, p4, IM_COL32(255, 255, 255, 255));
            drawList->AddText(ImVec2(x - 10, y - 20), IM_COL32(200, 255, 200, 255), notify.name.c_str());

            if (ImGui::BeginPopup(("DeleteNotifyPopup_" + std::to_string(i)).c_str()))
            {
                ImGui::Text("Notify: %s", notify.name.c_str());
                ImGui::Separator();
                if (ImGui::MenuItem("Delete"))
                {
                    notifies.erase(notifies.begin() + i);
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
                ImGui::EndPopup();
            }
        }

        static float rightClickTime = 0.0f;
        if (isRightClicked && hoveredNotifyIndex == -1)
        {
            float mouseRatio = (ImGui::GetMousePos().x - cursorPos.x) / width;
            rightClickTime = std::clamp(mouseRatio, 0.0f, 1.0f) * duration;

            ImGui::OpenPopup("TimelineContextMenu");
        }

        if (ImGui::BeginPopup("TimelineContextMenu"))
        {
            ImGui::Text("Add Notify at %.2f sec", rightClickTime);
            static char notifyNameBuf[64] = "Event";
            ImGui::InputText("Name", notifyNameBuf, 64);

            if (ImGui::Button("Add"))
            {
                AddNotify(animName, notifyNameBuf, rightClickTime);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (isHovered && isDown)
        {
            float mouseRatio = (ImGui::GetMousePos().x - cursorPos.x) / width;
            float newTime = std::clamp(mouseRatio, 0.0f, 1.0f) * duration;

            if (layer.current.name != animName || !layer.current.active)
            {
                Play(animName, false, targetLayerIndex, 0.0f);
            }

            layer.current.time = newTime;
            layer.current.speed = 0.0f;

            if (layer.next.active)
            {
                layer.next.Reset();
                layer.transitionTime = 0.0f;
            }

            displayTime = newTime;

            UpdateTimeinePose();
        }

        float playheadX = cursorPos.x + (displayTime / duration) * width;
        drawList->AddLine(ImVec2(playheadX, cursorPos.y), ImVec2(playheadX, cursorPos.y + height), IM_COL32(255, 50, 50, 255), 2.0f);
    
    }

    // Update()의 deltatime 제외 본 행렬 계산
    void SkeletalAnimator::UpdateTimeinePose()
    {
        if (m_skeleton.empty() || !m_skeletonData) return;

        const auto& boneOffsets = m_skeletonData->GetBoneOffsets();

        for (size_t i = 0; i < m_skeleton.size(); ++i)
        {
            auto& bone = m_skeleton[i];

            Vector3 finalPos = bone.local.Translation();
            Quaternion finalRot = Quaternion::CreateFromRotationMatrix(bone.local);
            Vector3 finalScale{ 1.0f, 1.0f, 1.0f };

            for (size_t layerIndex = 0; layerIndex < m_layers.size(); ++layerIndex)
            {
                auto& layer = m_layers[layerIndex];
                if (!layer.current.active) continue;

                if (!layer.mask.empty())
                {
                    if (i >= layer.mask.size() || layer.mask[i] == 0)
                    {
                        continue;
                    }
                }

                Vector3 curPos;
                Quaternion curRot;
                Vector3 curScale;

                bool hasCur = EvaluateBone(layer.current, bone.name, layer.current.time, curPos, curRot, curScale);

                if (!hasCur)
                {
                    continue;
                }

                Vector3 layerPos = curPos;
                Quaternion layerRot = curRot;
                Vector3 layerScale = curScale;

                if (layer.next.active)
                {
                    Vector3 nextPos;
                    Quaternion nextRot;
                    Vector3 nextScale;

                    bool hasNext = EvaluateBone(layer.next, bone.name, layer.next.time, nextPos, nextRot, nextScale);

                    if (hasNext)
                    {
                        float t = std::clamp(layer.transitionTime / layer.transitionDuration, 0.0f, 1.0f);

                        layerPos = Vector3::Lerp(layerPos, nextPos, t);
                        layerRot = Quaternion::Slerp(layerRot, nextRot, t);
                        layerScale = Vector3::Lerp(layerScale, nextScale, t);
                    }
                }

                float w = layer.weight;

                if (layer.blendMode == AnimationBlendMode::Additive)
                {
                    Vector3 refPos;
                    Quaternion refRot;
                    Vector3 refScale;

                    EvaluateBone(layer.current, bone.name, 0.0f, refPos, refRot, refScale);

                    Vector3 deltaPos = layerPos - refPos;
                    Quaternion inv;
                    refRot.Inverse(inv);
                    Quaternion deltaRot = layerRot * inv;

                    finalPos += deltaPos * w;
                    finalRot = finalRot * Quaternion::Slerp(Quaternion::Identity, deltaRot, w);
                }
                else
                {
                    if (layerIndex == 0)
                    {
                        finalPos = layerPos;
                        finalRot = layerRot;
                        finalScale = layerScale;
                    }
                    else
                    {
                        finalPos = Vector3::Lerp(finalPos, layerPos, w);
                        finalRot = Quaternion::Slerp(finalRot, layerRot, w);
                        finalScale = Vector3::Lerp(finalScale, layerScale, w);
                    }
                }
            }

            if (auto iter = m_proceduralRotations.find(static_cast<int>(i));
                iter != m_proceduralRotations.end())
            {
                finalRot = finalRot * iter->second;
            }

            Matrix nodeTrans =
                Matrix::CreateScale(finalScale) *
                Matrix::CreateFromQuaternion(finalRot) *
                Matrix::CreateTranslation(finalPos);

            if (bone.parentIndex != -1)
            {
                bone.model = nodeTrans * m_skeleton[bone.parentIndex].model;
            }
            else
            {
                bone.model = nodeTrans;
            }

            m_finalBoneMatrices[bone.index] = (boneOffsets[bone.index] * bone.model).Transpose();
        }

        auto renderer = GetGameObject()->GetComponent<SkeletalMeshRenderer>();
        if (renderer)
        {
            renderer->Update();
        }
    }

    const BoneMatrixArray& SkeletalAnimator::GetFinalBoneMatrices() const
    {
        return m_finalBoneMatrices;
    }

    void SkeletalAnimator::OnGui()
    {
        ImGui::Indent();
        
        // animation
        ImGui::Indent();
        if (ImGui::CollapsingHeader("Animation Registration", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static std::string selectedPath = "";
            static char nameBuf[128]{};

            ImGui::InputText("##Name", nameBuf, 128);

            std::string tempPath;
            if (DrawFileSelector("Select FBX", "Resource/Animation", ".fbx", tempPath))
            {
                selectedPath = tempPath;

                if (strlen(nameBuf) == 0)
                {
                    std::string filename = std::filesystem::path(selectedPath).filename().stem().string();
                    strcpy_s(nameBuf, filename.c_str());
                }
            }

            ImGui::Text("Selected: %s", selectedPath.empty() ? "(None)" : selectedPath.c_str());

            if (ImGui::Button("Add Animation"))
            {
                if (!selectedPath.empty())
                {
                    RegisterAnimation(nameBuf, selectedPath);
                    selectedPath = "";
                    nameBuf[0] = '\0';
                }
            }
        }
        ImGui::Unindent();

        ImGui::Indent();
        if (ImGui::CollapsingHeader("Registered List", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild("AnimList", ImVec2(0, 150), true);

            std::string removeTarget = "";

            for (auto& [name, res] : m_animations)
            {
                ImGui::PushID(name.c_str());

                bool isSelected = (m_selectedAnimName == name);
                if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_AllowItemOverlap))
                {
                    m_selectedAnimName = name;
                }

                // 우측에 삭제 버튼 배치
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30);
                if (ImGui::Button("X"))
                {
                    removeTarget = name;
                    if (m_selectedAnimName == name) m_selectedAnimName = "";
                }

                ImGui::PopID();
            }

            ImGui::EndChild();

            if (!removeTarget.empty())
            {
                UnregisterAnimation(removeTarget);
            }

            if (!m_selectedAnimName.empty())
            {
                if (m_animations.find(m_selectedAnimName) != m_animations.end())
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Timeline Editor : %s", m_selectedAnimName.c_str());

                    static char nameBuf[128] = "";
                    if (strcmp(nameBuf, m_selectedAnimName.c_str()) != 0 && !ImGui::IsAnyItemActive())
                    {
                        strcpy_s(nameBuf, m_selectedAnimName.c_str());
                    }

                    if (ImGui::InputText("Rename", nameBuf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        if (strcmp(nameBuf, m_selectedAnimName.c_str()) != 0)
                        {
                            RenameAnimation(m_selectedAnimName, nameBuf);
                            m_selectedAnimName = nameBuf;
                        }
                    }

                    ImGui::Spacing();

                    DrawTimeline(m_selectedAnimName);

                    ImGui::Spacing();
                    ImGui::Spacing();
                }
                else
                {
                    m_selectedAnimName = "";
                }
            }
        }
        ImGui::Unindent();

        // layer
        ImGui::Indent();
        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Add Layer"))
            {
                AddLayer("New Layer", 1.0f);
            }

            for (size_t i = 0; i < m_layers.size(); ++i)
            {
                auto& layer = m_layers[i];
                ImGui::PushID(static_cast<int>(i));

                bool open = ImGui::TreeNodeEx("##LayerNode", ImGuiTreeNodeFlags_AllowItemOverlap, "Layer %d", i);

                if (open)
                {
                    char layerName[64]{};
                    strcpy_s(layerName, layer.name.c_str());
                    ImGui::SetNextItemWidth(100);
                    if (ImGui::InputText("##LName", layerName, 64))
                    {
                        layer.name = layerName;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("X"))
                    {
                        RemoveLayer(static_cast<int>(i));
                        ImGui::PopID();

                        if (open)
                        {
                            ImGui::TreePop();
                            --i;
                            continue;
                        }
                    }

                    ImGui::DragFloat("Weight", &layer.weight, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                    int blend = static_cast<int>(layer.blendMode);
                    ImGui::RadioButton("Override", &blend, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("Additive", &blend, 1);
                    layer.blendMode = static_cast<AnimationBlendMode>(blend);

                    ImGui::Text("Playing: %s", layer.current.name.empty() ? "None" : layer.current.name.c_str());

                    if (ImGui::TreeNode("Mask Tree (Shift+Click: Recursive)"))
                    {
                        if (!m_skeleton.empty())
                        {
                            if (layer.mask.size() != m_skeleton.size())
                            {
                                layer.mask.resize(m_skeleton.size(), 0);
                            }

                            for (const auto& bone : m_skeleton)
                            {
                                if (bone.parentIndex == -1)
                                {
                                    RenderBoneTree(bone.index, layer.mask);
                                }
                            }
                        }

                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
        ImGui::Unindent();
        
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    void SkeletalAnimator::Save(json& j) const
    {
        Object::Save(j);

        // unordered_map을 vector로 옮겨서 이름순 정렬
        std::vector<std::pair<std::string, AnimationResource>> sortedAnims(
            m_animations.begin(), m_animations.end());

        // 이름(first) 기준으로 오름차순 정렬
        std::sort(sortedAnims.begin(), sortedAnims.end(),
            [](const auto& a, const auto& b)
            {
                return a.first < b.first;
            }
        );

        std::vector<json> animList;
        for (auto& [name, res] : sortedAnims)
        {
            json node;
            node["Name"] = name;
            node["Path"] = res.path;

            std::vector<json> notifyList;
            for (const auto& notify : res.notifies)
            {
                json notifyNode;
                notifyNode["Name"] = notify.name;
                notifyNode["Time"] = notify.time;
                notifyList.push_back(notifyNode);
            }
            node["Notifies"] = notifyList;

            animList.push_back(node);
        }

        j["Animations"] = animList;

        std::vector<json> layerList;
        for (const auto& layer : m_layers)
        {
            json node;
            node["Name"] = layer.name;
            node["Weight"] = layer.weight;
            node["BlendMode"] = layer.blendMode;

            std::vector<std::string> maskBones;
            if (!layer.mask.empty())
            {
                for (size_t k = 0; k < layer.mask.size() && k < m_skeleton.size(); ++k)
                {
                    if (layer.mask[k] != 0)
                    {
                        maskBones.push_back(m_skeleton[k].name);
                    }
                }
            }

            node["MaskBones"] = maskBones;
            layerList.push_back(node);
        }

        j["Layers"] = layerList;

    }

    void SkeletalAnimator::Load(const json& j)
    {
        Object::Load(j);

        JsonArrayForEach(j, "Animations", [&](const json& node)
            {
                std::string name = node.value("Name", "");
                std::string path = node.value("Path", "");

                RegisterAnimation(name, path);

                if (node.contains("Notifies"))
                {
                    for (const auto& notifyNode : node["Notifies"])
                    {
                        std::string notifyName = notifyNode.value("Name", "");
                        float time = notifyNode.value("Time", 0.0f);

                        AddNotify(name, notifyName, time);
                    }
                }
            }
        );

        m_layers.clear();

        JsonArrayForEach(j, "Layers", [&](const json& node)
            {
                AddLayer(node.value("Name", "Layer"), node.value("Weight", 1.0f));
                m_layers.back().blendMode = node.value("BlendMode", AnimationBlendMode::Override);

                m_layers.back().pendingMaskBones = node["MaskBones"];
            }
        );

        if (m_layers.empty())
        {
            AddLayer("Base Layer", 1.0f);
        }

        if (!m_skeleton.empty())
        {
            InitializeSkeleton();
        }
    }

    void SkeletalAnimator::InitializeSkeleton()
    {
        if (m_skeleton.empty())
        {
            if (auto renderer = GetGameObject()->GetComponent<SkeletalMeshRenderer>())
            {
                m_skeletonData = renderer->GetSkeletonData(); // 캐싱
                if (m_skeletonData)
                {
                    m_skeletonData->SetupSkeletonInstance(m_skeleton);
                }
            }
        }

        if (!m_skeleton.empty())
        {
            for (auto& layer : m_layers)
            {
                if (layer.mask.empty())
                {
                    layer.mask.assign(m_skeleton.size(), 0);
                }

                if (!layer.pendingMaskBones.empty())
                {
                    SetLayerMaskInternal(layer, layer.pendingMaskBones);
                    layer.pendingMaskBones.clear();
                }
            }
        }
    }

    void SkeletalAnimator::MarkBoneRecursive(std::vector<std::uint8_t>& mask, int parentIndex, bool active)
    {
        for (const auto& bone : m_skeleton)
        {
            if (bone.parentIndex == parentIndex)
            {
                mask[bone.index] = active ? 1 : 0;
                MarkBoneRecursive(mask, bone.index, active);
            }
        }
    }

    bool SkeletalAnimator::EvaluateBone(
        const AnimationState& state,
        const std::string& boneName,
        float time,
        Vector3& outPos,
        Quaternion& outRot,
        Vector3& outScale)
    {
        if (!state.data)
        {
            return false;
        }

        const auto& anims = state.data->GetAnimations();
        if (anims.empty())
        {
            return false;
        }

        const auto& clip = anims[0];

        auto iter = clip.animMappingTable.find(boneName);
        if (iter != clip.animMappingTable.end())
        {
            LastKeyIndex tempIndex{};
            clip.boneAnimations[iter->second].Evaluate(time, tempIndex, outPos, outRot, outScale);
            return true;
        }

        return false;
    }

    void SkeletalAnimator::RenderBoneTree(int boneIndex, std::vector<std::uint8_t>& mask)
    {
        if (boneIndex >= m_skeleton.size() || boneIndex >= mask.size())
        {
            return;
        }

        const auto& bone = m_skeleton[boneIndex];

        ImGui::PushID(boneIndex);

        bool isChecked = (mask[boneIndex] != 0);
        if (ImGui::Checkbox("", &isChecked))
        {
            mask[boneIndex] = isChecked ? 1 : 0;

            if (ImGui::GetIO().KeyShift)
            {
                MarkBoneRecursive(mask, boneIndex, isChecked);
            }
        }

        ImGui::SameLine();

        bool hasChildren = false;

        for (const auto& b : m_skeleton)
        {
            if (b.parentIndex == boneIndex)
            {
                hasChildren = true;
                break;
            }
        }

        if (hasChildren)
        {
            if (ImGui::TreeNode(bone.name.c_str()))
            {
                for (const auto& child : m_skeleton)
                {
                    if (child.parentIndex == boneIndex)
                    {
                        RenderBoneTree(child.index, mask);
                    }
                }

                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::Text(bone.name.c_str());
        }

        ImGui::PopID();
    }

    void SkeletalAnimator::SetLayerMaskInternal(AnimationLayer& layer, const std::vector<std::string>& boneNames)
    {
        std::fill(layer.mask.begin(), layer.mask.end(), 0);

        for (const auto& name : boneNames)
        {
            for (const auto& bone : m_skeleton)
            {
                if (bone.name == name)
                {
                    layer.mask[bone.index] = 1;

                    break;
                }
            }
        }
    }
    void SkeletalAnimator::CheckNotifies(const AnimationState& state, float prevTime, float currTime)
    {
        auto find = m_animations.find(state.name);
        if (find == m_animations.end()) return;

        const auto& notifies = find->second.notifies;
        if (notifies.empty()) return;

        float duration = 0.0f;
        if (state.data && !state.data->GetAnimations().empty())
        {
            duration = state.data->GetAnimations()[0].duration;
        }

        for (const auto& notify : notifies)
        {
            bool fired = false;

            if (prevTime < notify.time && notify.time <= currTime)
            {
                fired = true;
            }
            else if (currTime < prevTime)
            {
                if (prevTime < notify.time && notify.time <= duration)
                {
                    fired = true;
                }
                else if (0.0f <= notify.time && notify.time <= currTime)
                {
                    fired = true;
                }
            }

            if (fired)
            {
                auto iter = m_notifyCallbacks.find(notify.name);
                if (iter != m_notifyCallbacks.end())
                {
                    if (iter->second)
                    {
                        iter->second();
                    }
                }
            }
        }
    }
}