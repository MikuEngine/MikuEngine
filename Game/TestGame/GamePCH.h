#pragma once

#include <Engine/EnginePCH.h>

#include <Engine/Framework/Object/Ptr.h>
#include <Engine/Framework/Object/GameObject/GameObject.h>
#include <Engine/Framework/Object/Component/Transform.h>
#include <Engine/Framework/Object/Component/SpriteRenderer.h>
#include <Engine/Framework/Object/Component/StaticMeshRenderer.h>
#include <Engine/Framework/Object/Component/SkeletalMeshRenderer.h>
#include <Engine/Framework/Object/Component/SkeletalAnimator.h>
#include <Engine/Framework/Object/Component/SpriteAnimator.h>
#include <Engine/Framework/Object/Component/Camera.h>
#include <Engine/Framework/Object/Component/Light.h>

// Physics Components
#include <Engine/Framework/Object/Component/Rigidbody.h>
#include <Engine/Framework/Object/Component/BoxCollider.h>
#include <Engine/Framework/Object/Component/SphereCollider.h>
#include <Engine/Framework/Object/Component/CapsuleCollider.h>
#include <Engine/Framework/Object/Component/CharacterController.h>

// Sound Component
#include <Engine/Framework/System/SoundSystem.h>
#include <Engine/Framework/Object/Component/AudioSource.h>

// Game Scripts
#include "Script/InputBinding.h"
#include "Script/CharacterLogicFSM.h"
#include "Script/CharacterAnimationFSM.h"
#include "Script/PlayerFSM.h"
#include "Script/TestLogicFSM.h"
#include "Script/TestAnimationFSM.h"