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

float4 main(PS_INPUT_TEXCOORD input) : SV_Target
{
    float2 uv = input.texCoord;
    float depth = g_gBufferDepth.Sample(g_samPoint, uv).r;
    if (depth >= 1.0f)
    {
        discard;
    }
    
    float3 baseColor = g_gBufferBaseColor.Sample(g_samPoint, uv).rgb;
    float4 encodedNormal = g_gBufferNormal.Sample(g_samPoint, uv).rgba;
    float lightingFlag = encodedNormal.a;
    if (lightingFlag < 0.1f)
    {
        return float4(baseColor, 1.0f);
    }
    
    float3 emissive = g_gBufferEmissive.Sample(g_samPoint, uv).rgb;
    float3 orm = g_gBufferORM.Sample(g_samPoint, uv).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metalness = orm.b;
    
    // world position
    float4 clipPosition;
    clipPosition.x = uv.x * 2.0f - 1.0f;
    clipPosition.y = -(uv.y * 2.0f - 1.0f);
    clipPosition.z = depth;
    clipPosition.w = 1.0f;
    
    float4 worldPosition = mul(clipPosition, g_invViewProjection);
    worldPosition /= worldPosition.w;
    
    // normal
    float3 n = normalize(DecodeNormal(encodedNormal.rgb));
    
    // view
    float3 v = normalize(g_cameraWorldPosition - worldPosition.xyz);
    
    // light
    float3 l = -normalize(g_mainLightWorldDirection);
    
    // l-v half
    float3 h = normalize(l + v);
    
    float nDotH = saturate(dot(n, h));
    
    float nDotL = saturate(dot(n, l));
    
    float nDotV = saturate(dot(n, v));
    
    float hDotV = saturate(dot(h, v));
    
    // shadow
    float4 lightClipPos = mul(float4(worldPosition.xyz, 1.0f), g_mainLightViewProjection);
    
    float shadowFactor = 1.0f;
    float currentShadowDepth = lightClipPos.z / lightClipPos.w;
    float2 shadowMapUV = lightClipPos.xy / lightClipPos.w;
    
    shadowMapUV.y = -shadowMapUV.y;
    shadowMapUV = shadowMapUV * 0.5f + 0.5f;
    
    if (all(shadowMapUV >= 0.0f) && all(shadowMapUV <= 1.0f))
    {
        if (g_useShadowPCF)
        {
            if (currentShadowDepth > 1.0f)
            {
                shadowFactor = 1.0f;
            }
            else
            {
                float texelSize = 1.0f / g_shadowMapSize;

                int max = g_pcfSize;
                float sum = 0.0f;
                for (int y = -max; y <= max; ++y)
                {
                    for (int x = -max; x <= max; ++x)
                    {
                        float2 offset = float2(x, y) * texelSize;
                        float2 sampleUV = shadowMapUV + offset;

                        sum += g_texShadowMap.SampleCmpLevelZero(g_samComparison, sampleUV, currentShadowDepth - 0.00001f);
                    }
                }
                shadowFactor = sum / ((max * 2 + 1) * (max * 2 + 1));
            }
        }
        else
        {
            float sampleShadowDepth = g_texShadowMap.Sample(g_samLinear, shadowMapUV).r;
            if (currentShadowDepth > 1.0f)
            {
                shadowFactor = 1.0f;
            }
            else if (currentShadowDepth > sampleShadowDepth + 0.001f)
            {
                shadowFactor = 0.0f;
            }
        }
    }
    
    float3 f0 = lerp(DielectricFactor, baseColor.rgb, metalness);
    
    float g = GAFSchlickGGX(nDotV, nDotL, roughness);
    
    float3 directLighting = 0.0f;
    {
        float d = NDFGGXTR(nDotH, max(0.04f, roughness));
        
        float3 f = FresnelSchlick(f0, hDotV);
        
        float3 kd = lerp(1.0f - f, 0.0f, metalness);
        
        float3 diffuseBRDF = kd * baseColor.rgb / PI;
        float3 specularBRDF = (f * d * g) / max(EPSILON, 4.0f * nDotL * nDotV);
    
        directLighting = (diffuseBRDF + specularBRDF) * g_mainLightColor * g_mainLightIntensity * nDotL * shadowFactor;
    }
    
    float3 ambientLighting = 0.0f;
    if (g_useIBL)
    {
        float3 f = FresnelSchlick(f0, nDotV);
        
        float3 kd = lerp(1.0f - f, 0.0f, metalness);
        
        float3 irradiance = g_texIBLIrradiance.Sample(g_samLinear, n).rgb;
    
        float3 diffuseIBL = kd * baseColor / PI * irradiance;
    
        uint specularTextureLevels, width, height;
        g_texIBLSpecular.GetDimensions(0, width, height, specularTextureLevels);
    
        float3 viewReflect = -(v - 2.0 * nDotV * n);
    
        float3 prefilteredColor = g_texIBLSpecular.SampleLevel(g_samLinear, viewReflect, roughness * specularTextureLevels).rgb;
    
        float2 specularBRDF = g_texIBLSpecularBRDFLUT.Sample(g_samClamp, float2(nDotV, roughness)).rg;
    
        float3 specularIBL = prefilteredColor * (f0 * specularBRDF.x + specularBRDF.y);
        
        ambientLighting = (diffuseIBL + specularIBL) * ao;
    }
    
    float3 final = directLighting + ambientLighting + emissive;
    
    return float4(final, 1.0f);
}