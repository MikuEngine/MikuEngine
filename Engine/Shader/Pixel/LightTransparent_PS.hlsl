#include "../Include/Shared.hlsli"
#include "../Include/PBR_Shared.hlsli"

float4 main(PS_INPUT_GBUFFER input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    float2 uv = input.texCoord;
    float4 baseColor = g_texBaseColor.Sample(g_samLinear, uv);
    baseColor = float4(pow(abs(baseColor.rgb), 2.2f), baseColor.a);
    float3 encodedNormal = g_texNormal.Sample(g_samLinear, uv).rgb;
    float3 worldPosition = input.worldPosition;
    float3 emissive = pow(abs(g_texEmissive.Sample(g_samLinear, uv).rgb), 2.2f);
    float ao = g_texAmbientOcclusion.Sample(g_samLinear, uv).r;
    float roughness = g_texRoughness.Sample(g_samLinear, uv).r;
    float metalness = g_texMetalness.Sample(g_samLinear, uv).r;
    
    // Material override 및 조정
    baseColor *= g_materialBaseColor;
    baseColor.a *= g_materialAlpha; // 장애물 반투명 적용
    emissive = emissive * g_materialEmissive * g_materialEmissiveIntensity;
    
    if (g_overrideMaterial)
    {
        baseColor = g_materialBaseColor;
        baseColor.a *= g_materialAlpha; // 장애물 반투명 적용
        emissive = 0.0f;
        ao = g_materialAmbientOcclusion;
        roughness = g_materialRoughness;
        metalness = g_materialMetalness;
    }
    else
    {
        // Override가 아닐 때는 텍스처 값에 머테리얼 값을 적용
        roughness = roughness * g_materialRoughness;
        metalness = saturate(metalness + g_materialMetalness);
        ao = ao * g_materialAmbientOcclusion;
    }
    
    // normal
    float3x3 tbn = float3x3(
        normalize(input.tangent),
        normalize(input.binormal),
        normalize(input.normal)
    );
    
    float3 n = normalize(mul(DecodeNormal(encodedNormal), tbn));
    
    // 반대면(backface)일 때 normal을 뒤집어서 올바른 라이팅 계산
    if (!isFrontFace)
    {
        n = -n;
    }
    
    // view
    float3 v = normalize(g_cameraWorldPosition - worldPosition);
    
    // light
    float3 l = -normalize(g_mainLightWorldDirection);
    
    // l-v half
    float3 h = normalize(l + v);
    
    float nDotH = saturate(dot(n, h));
    
    float nDotL = saturate(dot(n, l));
    
    float nDotV = saturate(dot(n, v));
    
    float hDotV = saturate(dot(h, v));
    
    // shadow
    float4 lightClipPos = mul(float4(worldPosition, 1.0f), g_mainLightViewProjection);
    
    float shadowFactor = 1.0f;
    //float currentShadowDepth = lightClipPos.z / lightClipPos.w;
    //float2 shadowMapUV = lightClipPos.xy / lightClipPos.w;
    
    //shadowMapUV.y = -shadowMapUV.y;
    //shadowMapUV = shadowMapUV * 0.5f + 0.5f;
    
    //if (all(shadowMapUV >= 0.0f) && all(shadowMapUV <= 1.0f))
    //{
    //    if (g_useShadowPCF)
    //    {
    //        if (currentShadowDepth > 1.0f)
    //        {
    //            shadowFactor = 1.0f;
    //        }
    //        else
    //        {
    //            float texelSize = 1.0f / g_shadowMapSize;

    //            int max = g_pcfSize;
    //            float sum = 0.0f;
    //            for (int y = -max; y <= max; ++y)
    //            {
    //                for (int x = -max; x <= max; ++x)
    //                {
    //                    float2 offset = float2(x, y) * texelSize;
    //                    float2 sampleUV = shadowMapUV + offset;

    //                    sum += g_texShadowMap.SampleCmpLevelZero(g_samComparison, sampleUV, currentShadowDepth - 0.0001f);
    //                }
    //            }
    //            shadowFactor = sum / ((max * 2 + 1) * (max * 2 + 1));
    //        }
    //    }
    //    else
    //    {
    //        float sampleShadowDepth = g_texShadowMap.Sample(g_samLinear, shadowMapUV).r;
    //        if (currentShadowDepth > 1.0f)
    //        {
    //            shadowFactor = 1.0f;
    //        }
    //        else if (currentShadowDepth > sampleShadowDepth + 0.001f)
    //        {
    //            shadowFactor = 0.0f;
    //        }
    //    }
    //}
    
    float3 f0 = lerp(DielectricFactor, baseColor.rgb, metalness);
    
    float g = GAFSchlickGGX(nDotV, nDotL, roughness);
    
    float3 directLighting = 0.0f;
    {
        float d = NDFGGXTR(nDotH, max(0.1f, roughness));
        
        float3 f = FresnelSchlick(f0, hDotV);
        
        float3 kd = lerp(1.0f - f, 0.0f, metalness);
        
        float3 diffuseBRDF = kd * baseColor.rgb / PI;
        float3 specularBRDF = (f * d * g) / max(EPSILON, 4.0f * nDotL * nDotV);
    
        directLighting = (diffuseBRDF + specularBRDF) * g_mainLightColor * g_mainLightIntensity * nDotL * shadowFactor;
    }
    
    float3 ambientLighting = 0.0f;
    if (g_useIBL)
    {
        float3 f = FresnelSchlickRoughness(f0, nDotV, roughness);
        
        float3 kd = lerp(1.0f - f, 0.0f, metalness);
        
        float3 irradiance;
        float3 prefilteredColor;
        
        float3 viewReflect = reflect(-v, n);
        
        if (g_useIBLTexture > 0.5f)
        {
            // 텍스처 사용
            irradiance = g_texIBLIrradiance.Sample(g_samLinear, n).rgb;
            
            uint specularTextureLevels, width, height;
            g_texIBLSpecular.GetDimensions(0, width, height, specularTextureLevels);
            
            
            prefilteredColor = g_texIBLSpecular.SampleLevel(g_samLinear, viewReflect, roughness * (specularTextureLevels - 1)).rgb;
        }
        else
        {
            // 색상 사용
            irradiance = g_iblAmbientColor;
            prefilteredColor = g_iblAmbientColor;
        }
    
        float3 diffuseIBL = kd * baseColor.rgb / PI * irradiance;
    
        float2 specularBRDF = g_texIBLSpecularBRDFLUT.Sample(g_samClamp, float2(nDotV, roughness)).rg;
    
        float specularSuppressor = saturate(1.0f - roughness * roughness);
        float specularIBLScale = lerp(specularSuppressor, 1.0f, metalness);

        float3 specularIBL = prefilteredColor * (f0 * specularBRDF.x + specularBRDF.y) * specularIBLScale;

        float horizonOcclusion = saturate(1.0f + dot(viewReflect, n));
        horizonOcclusion *= horizonOcclusion;
        specularIBL *= horizonOcclusion;

        ambientLighting = (diffuseIBL + specularIBL) * ao;
    }
    
    float3 final = directLighting + ambientLighting + emissive;
    
    return float4(final, baseColor.a);
}