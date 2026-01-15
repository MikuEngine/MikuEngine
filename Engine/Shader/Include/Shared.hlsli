#ifndef SHARED_HLSLI
#define SHARED_HLSLI

// pbr material textures
Texture2D g_texBaseColor                : register(t0);
Texture2D g_texNormal                   : register(t1);
Texture2D g_texMetalness                : register(t2);
Texture2D g_texRoughness                : register(t3);
Texture2D g_texAmbientOcclusion         : register(t4);
Texture2D g_texEmissive                 : register(t5);

// g-buffer textures
Texture2D g_gBufferBaseColor            : register(t10);
Texture2D g_gBufferNormal               : register(t11);
Texture2D g_gBufferORM                  : register(t12); // AO(r), roughness(g), metalness(b)
Texture2D g_gBufferEmissive             : register(t13);
Texture2D g_gBufferDepth                : register(t14);

// global
Texture2D g_texShadowMap                : register(t20);
TextureCube g_texIBLEnvironment         : register(t21);
TextureCube g_texIBLIrradiance          : register(t22);
TextureCube g_texIBLSpecular            : register(t23);
Texture2D g_texIBLSpecularBRDFLUT       : register(t24);

// utility
Texture2D g_texBlit                     : register(t30);
Texture2D g_texHDR                      : register(t31);
Texture2D<uint2> g_texStencilMap        : register(t32);

SamplerState g_samLinear                : register(s0);
SamplerState g_samPoint                 : register(s1);
SamplerComparisonState g_samComparison  : register(s2);
SamplerState g_samClamp                 : register(s3);



cbuffer Frame : register(b0) // 프레임 당 한번만 갱신되는 버퍼
{
    matrix g_view;
    
    matrix g_projection;
    
    matrix g_viewProjection;
    
    matrix g_invViewProjection;
    
    matrix g_mainLightViewProjection;
    
    float3 g_cameraWorldPosition;
    float g_elapsedTime;
    
    float3 g_mainLightWorldDirection;
    float g_mainLightIntensity;
    
    float3 g_mainLightColor;
    float g_maxHDRNits;
    
    float g_exposure;
    int g_shadowMapSize;
    int g_useShadowPCF;
    int g_pcfSize;
    
    int g_useIBL;
    float g_bloomStrength;
    float g_bloomThreshold;
    float g_bloomSoftKnee;
    
    float g_fxaaQualitySubpix; // 0.0 to 1.0 (default: 0.75)
    float g_fxaaQualityEdgeThreshold; // 0.063 to 0.333 (default: 0.166)
    float g_fxaaQualityEdgeThresholdMin; // 0.0312 to 0.0833 (default: 0.0833)
    float __pad1_Frame;
}

cbuffer Material : register(b1)
{
    float4 g_materialBaseColor;
    
    float3 g_materialEmissive;
    float g_materialEmissiveIntensity;
    
    float g_materialRoughness;
    float g_materialMetalness;
    float g_materialAmbientOcclusion;
    int g_overrideMaterial;
}

cbuffer Object : register(b2)
{
    matrix g_world;
    
    matrix g_worldInverseTranspose;
    
    int g_boneIndex;
    float3 __pad1_object;
}

cbuffer Bone : register(b3)
{
    matrix g_boneTransform[128];
}

cbuffer Blur : register(b4)
{
    float2 g_blurDir;
    float2 __pad1_Blur;
}

cbuffer Sprite : register(b5)
{
    float2 g_uvOffset;
    float2 g_uvScale;
    float2 g_pivot;
    float2 __pad1_Sprite;
};

cbuffer LocalLight : register(b6)
{
    float3 g_lightColor;
    float g_lightIntensity;
    
    float3 g_lightPosition;
    float g_lightRange;
    
    float3 g_lightDirection;
    float g_lightAngle;
};

cbuffer ScreenSize : register(b7)
{
    float2 g_screenSize;
    float2 __pad1_ScreenSize;
};

struct VS_INPUT_POSITION
{
    float3 position : POSITION;
};

struct PS_INPUT_LOCAL_POSITION
{
    float4 position : SV_Position;
    float3 localPosition : TEXCOORD0;
};

struct VS_INPUT_TEXCOORD
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PS_INPUT_TEXCOORD
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

struct VS_INPUT_SKINNING
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    uint4 blendIndices : BLENDINDICES;
    float4 blendWeights : BLENDWEIGHT;
};

struct VS_INPUT_COMMON
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct PS_INPUT_GBUFFER
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 binormal : TEXCOORD3;
    float3 worldPosition : TEXCOORD4;
};

struct PS_OUTPUT_GBUFFER
{
    float4 baseColor : SV_Target0;
    float4 normal : SV_Target1;
    float4 orm : SV_Target2;
    float4 emissive : SV_Target3;
};

struct PS_INPUT
{
    float4 position : SV_Position;
};

static const float PI = 3.141592f;
static const float EPSILON = 0.00001f;
static const float3 DielectricFactor = float3(0.04f, 0.04f, 0.04f);
static const matrix Identity = matrix(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
);

float3 EncodeNormal(float3 n)
{
    return n * 0.5f + 0.5f;
}

float3 DecodeNormal(float3 n)
{
    return n * 2.0f - 1.0f;
}

float GetLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 LinearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0f / 2.2f);
}

float3 SRGBToLinear(float3 linearColor)
{
    return pow(linearColor, 2.2f);
}

#endif //SHARED_HLSLI