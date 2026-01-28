#include "GamePCH.h"
#include "AnimationPlayer.h"

#include "Framework/Object/Component/Animator/SkeletalAnimator.h"
#include "Framework/Object/Component/Renderer/StaticMeshRenderer.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/System/SoundSystem.h"

namespace game
{
    void AnimationPlayer::Awake()
    {
		/*/ notify 테스트 코드
        auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;

        animator->BindNotify("PunchSound", [this]()
            {
                // 펀치 소리 재생 로직만 작성
                LOG_INFO("AnimationPlayer::Awake() Notify  퍽!");
                engine::SoundSystem::Get().Play("UI_Click_Random");
            });

        animator->BindNotify("CreateHitBox", [this]()
            {
                // 히트박스 생성 로직만 작성
                LOG_INFO("AnimationPlayer::Awake() Notify  공격 판정 ON");
            });
		//*/
    }

    void AnimationPlayer::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("1", 0.5f, true, 0, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("2", 0.5f, true, 0, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D3))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("3", 0.5f, true, 0, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D4))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("4", 0.5f, false, 1, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D5))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("5", 0.5f, false, 1, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D6))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            if (animator)
            {
                // 본의 로컬 위치와 회전 행렬 가져오기 (메시 로컬 스페이스)
                auto boneLocalPosition = animator->GetBoneWorldPosition("mixamorig:RightHand");
                auto boneLocalMatrix = animator->GetBoneWorldMatrix("mixamorig:RightHand");
                
                // GameObject의 Transform 가져오기
                auto transform = GetGameObject()->GetTransform();
                auto objectWorldMatrix = transform->GetWorld();
                
                // 본의 실제 월드 위치 계산 (GameObject의 Transform 적용)
                engine::Vector4 boneWorldPos4 = engine::Vector4::Transform(
                    engine::Vector4(boneLocalPosition.x, boneLocalPosition.y, boneLocalPosition.z, 1.0f),
                    objectWorldMatrix
                );
                engine::Vector3 boneWorldPosition = engine::Vector3(boneWorldPos4.x, boneWorldPos4.y, boneWorldPos4.z);
                
                // GameObject의 회전을 사용하여 offset 방향 계산
                engine::Vector3 objectForward = transform->GetForward();
                objectForward.Normalize();
                
                // offset 거리 설정
                float offset = 10.5f;
                
                // 본의 월드 위치에서 GameObject의 회전 방향으로 offset만큼 이동한 위치 계산
                auto finalPosition = boneWorldPosition + objectForward * offset;
                
                auto go = CreateGameObject();
                go->GetTransform()->SetLocalPosition(finalPosition);
                go->GetTransform()->SetLocalScale(5.5f);
                
                // 본의 회전도 적용 (GameObject의 회전과 본의 회전 결합)
                engine::Matrix boneWorldMatrix = boneLocalMatrix * objectWorldMatrix;
                engine::Quaternion boneRotation = engine::Quaternion::CreateFromRotationMatrix(boneWorldMatrix);
                go->GetTransform()->SetLocalRotation(boneRotation);
                
                auto sr = go->AddComponent<engine::StaticMeshRenderer>();
                sr->SetMesh("Resource/Model/Cube.fbx");
            }
        }
    }

    void AnimationPlayer::OnGui()
    {
    }

    void AnimationPlayer::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void AnimationPlayer::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}