#ifndef SHARED_HLSLI
#define SHARED_HLSLI

Texture2D g_texDiffuse              : register(t0);
Texture2D g_texNormal               : register(t1);
Texture2D g_texSpecular             : register(t2);
Texture2D g_texEmissive             : register(t3);
Texture2D g_texOpacity              : register(t4);
Texture2D g_texMetalness            : register(t5);
Texture2D g_texRoughness            : register(t6);
Texture2D g_texAmbientOcclusion     : register(t7);
Texture2D g_texShadowMap            : register(t8);
TextureCube g_texIblIrradiance      : register(t9);
TextureCube g_texIblSpecular        : register(t10);
Texture2D g_texIblSpecularBrdfLut   : register(t11);

SamplerState g_samLinear                : register(s0);
SamplerComparisonState g_samComparison  : register(s1);

cbuffer Transform : register(b0)
{
    matrix g_view;
    matrix g_projection;
    matrix g_lightView;
    matrix g_lightProjection;
}

cbuffer Environment : register(b1)
{
    float3 g_cameraPos;
    float __pad3;
    float3 g_lightDir;
    float __pad4;
    float4 g_lightColor;
    float4 g_ambientLightColor;
    int g_shadowMapSize;
    int g_useShadowPCF;
    int g_pcfSize;
    int g_useIBL;
}

cbuffer Material : register(b2)
{
    float4 g_materialDiffuse;
    float4 g_materialAmbient;
    float4 g_materialSpecular;
    float g_shininess;
    float __pad2[3];
}

cbuffer BonePoseMatrix : register(b3)
{
    matrix g_bonePose[128];
}

cbuffer BoneOffsetMatrix : register(b4)
{
    matrix g_boneOffset[128];
}

cbuffer WorldTransform : register(b5)
{
    matrix g_world;
    uint g_refBoneIndex;
    float __pad1[3];
}

cbuffer OverrideMaterial : register(b6)
{
    float4 g_overrideBaseColor;
    float g_overrideMetalness;
    float g_overrideRoughness;
    int g_overrideMaterial;
    float g_ambientOcclusion;
}

cbuffer HDR : register(b7)
{
    float g_exposure;
    float g_maxHDRNits;
}

struct VS_INPUT_SKINNING
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
    float3 norm : NORMAL;
    float3 tan : TANGENT;
    float3 binorm : BINORMAL;
    uint4 blendIndices : BLENDINDICES;
    float4 blendWeights : BLENDWEIGHT;
};

struct VS_INPUT_COMMON
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
    float3 norm : NORMAL;
    float3 tan : TANGENT;
    float3 binorm : BINORMAL;
};

struct PS_INPUT
{
    float4 pos : SV_Position;
    float2 tex : TEXCOORD0;
    float3 norm : TEXCOORD1;
    float3 tan : TEXCOORD2;
    float3 binorm : TEXCOORD3;
    float3 worldPos : TEXCOORD4;
};

struct PS_INPUT_SHADOW
{
    float4 pos : SV_Position;
    float2 tex : TEXCOORD0;
    float3 norm : TEXCOORD1;
    float3 tan : TEXCOORD2;
    float3 binorm : TEXCOORD3;
    float3 worldPos : TEXCOORD4;
    float4 lightViewPos : TEXCOORD5;
};

static const float PI = 3.141592f;
static const float EPSILON = 0.00001f;

float3 EncodeNormal(float3 n)
{
    return n * 0.5f + 0.5f;
}

float3 DecodeNormal(float3 n)
{
    return n * 2.0f - 1.0f;
}

#endif //SHARED_HLSLI