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
		Matrix invView;

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
		float __pad_fxaa;
		
		// 스카이박스/IBL 설정
		Vector3 skyboxColor;
		float useSkyboxTexture;  // 1.0f = true, 0.0f = false
		
		Vector3 skyboxHorizonColor;
		float useIBLTexture;
		
		Vector3 iblAmbientColor;
		float __pad_skybox1;
		
		// 포스트프로세싱 추가 설정
		float enableBloom;  // 1.0f = true, 0.0f = false
		float enableToneMapping;
		float enableFXAA;
		float __pad_postprocess1;

		// SSS (Subsurface Scattering) - 전역 상수. 0이면 비활성
		Vector3 subsurfaceColor;
		float subsurfaceStrength;
	};

	struct CbMaterial
	{
		Vector4 materialBaseColor;

		Vector3 materialEmissive;
		float materialEmissiveIntensity;

		float materialRoughness;
		float materialMetalness;
		float materialAmbientOcclusion;
		float materialAlpha; // 장애물 반투명용 (0.0 ~ 1.0)

		int overrideMaterial;
		Vector3 materialSubsurfaceColor;   // SSS tint per renderer (RGB), written to GBuffer subsurface RT

		float materialSubsurfaceStrength;  // SSS per-material strength (0 = off), written to GBuffer ORM.a
		float __pad[3];
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

		int localLightShadowIndex;
		int useLocalLightShadow;
		int spotShadowIndex;
		int useSpotShadow;

		Matrix spotLightViewProjection; // for spot shadow sampling (when useSpotShadow)
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

	struct CbPickingId
	{
		unsigned int pickingId;
		float __pad1[3];

	};

	struct CbUIElement
	{
		Matrix clip;        // 최종 SV_Position용
		Vector4 color;      // RGBA
		Vector4 uv;         // offset.xy, scale.xy
		Vector4 clipRect;   // (xMin, yMin, xMax, yMax) in pixels 또는 NDC 중 하나로 통일

		uint32_t maskMode;  // none, rect, circle, ring, rectring, radial
		uint32_t effectMode;     // 4
		uint32_t effectFlags;    // 4
		float    time;

		Vector4 mask0;		// (cx, cy, rInner, rOuter) 또는 (cx,cy,r,0)
		Vector4 mask1;		// (startAngleRad, fill01, clockwise, unused)

		float outlineThickness;
		float outlineEnabled;
		float __pad1[2];
		Vector4 outlineColor;

		// ---- Effect params (범용) ----
		Vector4 effect0;         // 16
		Vector4 effect1;         // 16
		Vector4 effect2;         // 16 
	};


	struct CbShadowPoint
	{
		Matrix viewProjections[6];
		Vector3 shadowLightPosition;
		float shadowLightRange;
		int shadowLightIndex;
		float __pad[3];
	};

	struct CbShadowSpot
	{
		Matrix spotLightViewProjection;
	};

}
#pragma warning(pop)