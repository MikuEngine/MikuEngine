#include "../Include/Shared.hlsli"
#include "../Include/PBR_Shared.hlsli"

float4 main(PS_INPUT input) : SV_Target
{
    float2 uv = input.position.xy / g_screenSize;
    float depth = g_gBufferDepth.Sample(g_samPoint, uv).r;
    if (depth >= 1.0f)
    {
        discard;
    }
    
    float4 encodedNormal = g_gBufferNormal.Sample(g_samPoint, uv).rgba;
    float lightingFlag = encodedNormal.a;
    if (lightingFlag < 0.1f)
    {
        discard;
    }
    
    // world position
    float4 clipPosition;
    clipPosition.x = uv.x * 2.0f - 1.0f;
    clipPosition.y = -(uv.y * 2.0f - 1.0f);
    clipPosition.z = depth;
    clipPosition.w = 1.0f;
    
    float4 worldPosition = mul(clipPosition, g_invViewProjection);
    worldPosition /= worldPosition.w;
    
    // light
    float3 l = g_lightPosition - worldPosition.xyz;
    float dist = length(l);
    
    if (dist > g_lightRange)
    {
        discard;
    }
    
    if (g_useLocalLightShadow)
    { // 섀도우 맵 샘플링 (저장된 값 = 정규화된 가장 가까운 거리)
        float closestDistNorm = g_texPointShadowMap.Sample(g_samLinear, float4(normalize(-l), (float) g_localLightShadowIndex)).r;
        float closestDist = closestDistNorm * g_lightRange;
    
    // Bias 적용 비교
        if (dist > closestDist + 0.1f) // Bias
        {
            discard;
        }
    }
    
    float3 baseColor = g_gBufferBaseColor.Sample(g_samPoint, uv).rgb;
    
    float3 orm = g_gBufferORM.Sample(g_samPoint, uv).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metalness = orm.b;
    
    // normal
    float3 n = normalize(DecodeNormal(encodedNormal.rgb));
    
    // view
    float3 v = normalize(g_cameraWorldPosition - worldPosition.xyz);
    
    l /= max(dist, EPSILON);
    
    float rawNDotL = dot(n, l);
    // 0.0을 기준으로 빛을 받으면 1, 아니면 0으로 딱 끊어버림
    float nDotL = rawNDotL > 0.0f ? 1.0f : 0.0f;
    
    //float nDotL = saturate(dot(n, l));
    
    if (nDotL <= 0.0f)
    {
        discard;
    }
    
    // l-v half
    float3 h = normalize(l + v);
    
    float attenuation = saturate(1.0f - (dist / g_lightRange));
    attenuation *= attenuation; // 제곱 감쇠
    
    float nDotH = saturate(dot(n, h));
    
    float nDotV = saturate(dot(n, v));
    
    float hDotV = saturate(dot(h, v));
    
    float3 f0 = lerp(DielectricFactor, baseColor.rgb, metalness);
    
    float g = GAFSchlickGGX(nDotV, nDotL, roughness);
        
    float d = NDFGGXTR(nDotH, max(0.04f, roughness));
        
    float3 f = FresnelSchlick(f0, hDotV);
        
    float3 kd = lerp(1.0f - f, 0.0f, metalness);
        
    float3 diffuseBRDF = kd * baseColor.rgb / PI;
    float3 specularBRDF = (f * d * g) / max(EPSILON, 4.0f * nDotL * nDotV);
        
    float3 final = (diffuseBRDF + specularBRDF) * g_lightColor * g_lightIntensity * nDotL * attenuation;
    
    return float4(final, 1.0f);
}