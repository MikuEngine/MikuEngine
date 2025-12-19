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

#endif //SHARED_HLSLI