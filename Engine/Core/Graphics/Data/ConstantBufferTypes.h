#pragma once

#include <directxtk/SimpleMath.h>

#pragma warning(push)
#pragma warning(disable: 26495)

namespace engine
{
	using Vector2 = DirectX::SimpleMath::Vector2;
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Vector4 = DirectX::SimpleMath::Vector4;
	using Matrix = DirectX::SimpleMath::Matrix;

	struct CbGlobal
	{
		Matrix view;
		
		Matrix projection;

		Matrix viewProjection;

		Matrix mainLightViewProjection;

		Vector3 cameraWorldPoistion;
		float elapsedTime;

		Vector3 mainLightWorldDirection;
		float mainLightIntensity;

		Vector3 mainLightColor;
		float maxHDRNits;

		float exposure;
		int shadowMapSize;
		int useShadowPCF;
		int pcfSize;

		int useIBL;
		float __pad1[3];
	};

	struct CbMaterial
	{
		Vector4 materialBaseColor;

		Vector3 materialEmissive;
		float materialEmissiveIntensity;

		float materialRoughness;
		float materialMetalness;
		float materialAmbientOcclusion;
		int overrideMaterial;
	};

	struct CbObject
	{
		Matrix world;
		
		Matrix worldInverseTranspose;

		int boneIndex;
		float __pad1[3];
	};

	struct CbBone
	{
		Matrix boneTransform[128];
	};
}

#pragma warning(pop)