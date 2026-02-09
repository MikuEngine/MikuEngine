#ifndef PBR_SHARED_HLSLI
#define PBR_SHARED_HLSLI

#include "../Include/Shared.hlsli"

float NDFGGXTR(float nDotH, float roughness)
{
    float a = roughness * roughness;
    float aSq = a * a;
    
    float denom = (nDotH * nDotH) * (aSq - 1.0f) + 1.0f;
    
    return aSq / (PI * denom * denom);
}

float3 FresnelSchlick(float3 f0, float cosTheta)
{
    return f0 + (1.0f - f0) * pow(1.0f - cosTheta, 5.0f);
}

float3 FresnelSchlickRoughness(float3 f0, float cosTheta, float roughness)
{
    // roughness가 높을수록 1.0(흰색)이 아닌 f0에 가깝게 억제합니다.
    return f0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), f0) - f0) * pow(1.0f - cosTheta, 5.0f);
}

float GAFSchlickGGXSub(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0f - k) + k);
}

float GAFSchlickGGX(float nDotV, float nDotL, float roughness)
{
    float a = roughness + 1.0f;
    float k = (a * a) / 8.0f;
    
    return GAFSchlickGGXSub(nDotV, k) * GAFSchlickGGXSub(nDotL, k);
}

#endif //PBR_SHARED_HLSLI