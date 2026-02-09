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
    
    float pointShadowFactor = 1.0f;
    if (g_useLocalLightShadow)
    {
        float closestDistNorm = g_texPointShadowMap.Sample(g_samLinear, float4(normalize(-l), (float) g_localLightShadowIndex)).r;
        float closestDist = closestDistNorm * g_lightRange;
        if (dist > closestDist + g_shadowBias)
            pointShadowFactor = 0.0f;
    }
    
    float3 baseColor = g_gBufferBaseColor.Sample(g_samPoint, uv).rgb;
    
    float4 orm = g_gBufferORM.Sample(g_samPoint, uv);
    float ao = orm.r;
    float roughness = orm.g;
    float metalness = orm.b;
    float sssStrengthScale = orm.a;
    float3 sssTint = g_gBufferSubsurface.Sample(g_samPoint, uv).rgb;
    if (dot(sssTint, sssTint) < 1e-10f)
        sssTint = g_subsurfaceColor;
    
    // normal
    float3 n = normalize(DecodeNormal(encodedNormal.rgb));
    
    // view
    float3 v = normalize(g_cameraWorldPosition - worldPosition.xyz);
    
    l /= max(dist, EPSILON);
    
    float nDotL = saturate(dot(n, l));
    
    // l-v half
    float3 h = normalize(l + v);
    
    float attenuation = saturate(1.0f - (dist / g_lightRange));
    attenuation *= attenuation;
    
    float nDotH = saturate(dot(n, h));
    
    float nDotV = saturate(dot(n, v));
    
    float hDotV = saturate(dot(h, v));
    
    float3 f0 = lerp(DielectricFactor, baseColor.rgb, metalness);
    
    float g = GAFSchlickGGX(nDotV, nDotL, roughness);
        
    float d = NDFGGXTR(nDotH, max(0.04f, roughness));
        
    float3 f = FresnelSchlick(f0, hDotV);
        
    float3 kd = lerp(1.0f - f, 0.0f, metalness);
        
    float3 diffuseBRDF = kd * baseColor.rgb / PI;
    // 표준 Cook-Torrance BRDF: 분모에 충분한 epsilon 사용, nDotV가 너무 작을 때 specular 클램핑
    float specularDenom = max(0.001f, 4.0f * nDotL * nDotV);
    float3 specularBRDF = (f * d * g) / specularDenom;
    // roughness가 높고 테두리(nDotV 작음)일 때 specular 억제
    specularBRDF *= saturate(nDotV + roughness);
        
    float3 final = (diffuseBRDF + specularBRDF) * g_lightColor * g_lightIntensity * nDotL * pointShadowFactor * attenuation;

    // SSS (wrap diffuse) - per-material strength in ORM.a, softened in shadow
    float sssStrength = g_subsurfaceStrength * sssStrengthScale;
    if (sssStrength > 0.0f)
    {
        float wrap = 0.5f;
        float wrapNdotL = saturate((dot(n, l) + wrap) / (1.0f + wrap));
        float shadowFactorSSS = lerp(0.25f, 1.0f, pointShadowFactor);
        final += (sssTint * baseColor) * g_lightColor * g_lightIntensity * wrapNdotL * shadowFactorSSS * attenuation * sssStrength;
        if (nDotL < 0.0f)
        {
            float backlit = -nDotL;
            final += (sssTint * baseColor) * g_lightColor * g_lightIntensity * backlit * shadowFactorSSS * attenuation * sssStrength * 0.45f;
        }
    }

    return float4(final, 1.0f);
}