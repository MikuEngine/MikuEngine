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

	struct CbFrame
	{
		Matrix view;
		
		Matrix projection;

		Matrix viewProjection;

		Matrix invViewProjection;

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
		float bloomStrength;
		float bloomThreshold;
		float bloomSoftKnee;

		float fxaaQualitySubpix;           // 0.0 to 1.0 (default: 0.75)
		float fxaaQualityEdgeThreshold;    // 0.063 to 0.333 (default: 0.166)
		float fxaaQualityEdgeThresholdMin; // 0.0312 to 0.0833 (default: 0.0833)
		float pad1;
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

	struct CbBlur
	{
		Vector2 blurDir;
		float __pad[2];
	};

	struct CbSprite
	{
		Vector2 uvOffset;
		Vector2 uvScale;
		Vector2 pivot;
		float __pad[2];
	};

	struct CbLocalLight
	{
		Vector3 lightColor;
		float lightIntensity;

		Vector3 lightPosition;
		float lightRange;

		Vector3 lightDirection;
		float lightAngle;
	};

	struct CbScreenSize
	{
		Vector2 screenSize;
		float __pad[2];
	};

	struct CbGrid
	{
		Vector4 gridColor; // 그리드 색상

		float gridSpacing; // 격자 간격 (예: 1.0)
		float gridWidth; // 선 두께 (예: 0.02 ~ 0.05)
		float __pad1[2];
	};
}

#pragma warning(pop)