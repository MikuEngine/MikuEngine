#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_SHADOW input) : SV_Target
{
    float4 texDiffColor = g_texDiffuse.Sample(g_samLinear, input.tex);
    float4 texNormColor = g_texNormal.Sample(g_samLinear, input.tex);
    float4 texSpecColor = g_texSpecular.Sample(g_samLinear, input.tex);
    float4 texOpacColor = g_texOpacity.Sample(g_samLinear, input.tex);
    float4 texEmsvColor = g_texEmissive.Sample(g_samLinear, input.tex);
    
    //clip(texOpacColor.a - 0.5f);
    
    // emissive
    float4 emissive = texEmsvColor;
    
    // normal
    float3 norm = normalize(input.norm);
    float3 tan = normalize(input.tan);
    float3 binorm = normalize(input.binorm);
    float3x3 tbnMatrix = float3x3(tan, binorm, norm);
    
    float3 texNorm = DecodeNormal(texNormColor.rgb);
    float3 worldNorm = normalize(mul(texNorm, tbnMatrix));
    
    // ambient
    float4 ambient = texDiffColor * g_materialAmbient * g_ambientLightColor;
    
    //// shadow
    float shadowFactor = 1.0f;
    //float currentShadowDepth = input.lightViewPos.z / input.lightViewPos.w;
    //float2 shadowMapUV = input.lightViewPos.xy / input.lightViewPos.w;
    
    //shadowMapUV.y = -shadowMapUV.y;
    //shadowMapUV = shadowMapUV * 0.5f + 0.5f;
    
    //if (all(shadowMapUV >= 0.0f) && all(shadowMapUV.x <= 1.0f))
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
    //        else if (currentShadowDepth > sampleShadowDepth + 0.0001f)
    //        {
    //            shadowFactor = 0.0f;
    //        }
    //    }
    //}
    
    // diffuse
    float diffuseIntensity = saturate(dot(worldNorm, -g_lightDir)) * shadowFactor;
    float4 diffuse = texDiffColor * g_lightColor * diffuseIntensity;
    
    // specular
    float3 viewDir = normalize(g_cameraPos - input.worldPos);
    float3 halfVector = normalize(-g_lightDir + viewDir);
    float nDotH = saturate(dot(worldNorm, halfVector));
    float specularIntensity = diffuseIntensity > 0.0f ? pow(nDotH, g_shininess.x) : 0.0f;
    float4 specular = texSpecColor * g_materialSpecular * g_lightColor * specularIntensity;
    
    float4 finalColor = saturate(diffuse + ambient + specular + emissive);
    finalColor.a = texOpacColor.a;
    
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}