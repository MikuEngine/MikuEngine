#pragma once

#include <directxtk/SimpleMath.h>

enum class ConstantBufferSlot : UINT
{
	Transform = 0,
	Environment = 1,
	Material = 2,
	BonePoseMatrix = 3,
	BoneOffsetMatrix = 4,
	WorldTransform = 5
};

struct WorldTransformBuffer
{
	DirectX::SimpleMath::Matrix world;
	unsigned int refBoneIndex;
	float __pad1[3];
};

struct TransformBuffer
{
	DirectX::SimpleMath::Matrix view;
	DirectX::SimpleMath::Matrix projection;
	DirectX::SimpleMath::Matrix lightView;
	DirectX::SimpleMath::Matrix lightProjection;
};

struct EnvironmentBuffer
{
	DirectX::SimpleMath::Vector3 cameraPos;
	float __pad1;
	DirectX::SimpleMath::Vector3 lightDirection;
	float __pad2;
	DirectX::SimpleMath::Vector4 lightColor;
	DirectX::SimpleMath::Vector4 ambientLightColor;
	int shadowMapSize;
	int useShadowPCF = 1;
	int pcfSize = 1;
	int useIBL = 1;
};

struct MaterialBuffer
{
	DirectX::SimpleMath::Vector4 diffuse{ 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::SimpleMath::Vector4 ambient{ 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::SimpleMath::Vector4 specular{ 1.0f, 1.0f, 1.0f, 1.0f };
	float shininess = 32.0f;
	float __pad1[3];
};