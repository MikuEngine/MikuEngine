#include "../Include/Shared.hlsli"

PS_OUTPUT_GBUFFER main(PS_INPUT_GBUFFER input, bool isFrontFace : SV_IsFrontFace)
{
    PS_OUTPUT_GBUFFER output = (PS_OUTPUT_GBUFFER) 0;
    
    float2 uv = input.texCoord;
    
    output.baseColor = g_texBaseColor.Sample(g_samLinear, uv);
    clip(output.baseColor.a - 0.5f);
    
    output.baseColor = float4(pow(abs(output.baseColor.rgb), 2.2f), output.baseColor.a);
    
    float3 encodedNormal = g_texNormal.Sample(g_samLinear, uv).rgb;
    output.emissive = float4(pow(abs(g_texEmissive.Sample(g_samLinear, uv).rgb), 2.2f), 1.0f);
    float ao = g_texAmbientOcclusion.Sample(g_samLinear, uv).r;
    float roughness = g_texRoughness.Sample(g_samLinear, uv).r;
    float metalness = g_texMetalness.Sample(g_samLinear, uv).r;
    output.baseColor *= g_materialBaseColor;
    output.baseColor.a *= g_materialAlpha; // 장애물 반투명 적용
    output.emissive.rgb = output.emissive.rgb * g_materialEmissive * g_materialEmissiveIntensity;
    
    if (g_overrideMaterial)
    {
        output.baseColor = g_materialBaseColor;
        output.baseColor.a *= g_materialAlpha; // 장애물 반투명 적용
        output.emissive.rgb = g_materialEmissive * g_materialEmissiveIntensity;
        ao = g_materialAmbientOcclusion;
        roughness = g_materialRoughness;
        metalness = g_materialMetalness;
    }
    else
    {
        roughness = roughness * g_materialRoughness;
        metalness = saturate(metalness + g_materialMetalness);
        ao = ao * g_materialAmbientOcclusion;
    }
    
    output.orm.r = ao;
    output.orm.g = roughness;
    output.orm.b = metalness;
    float thicknessScale = g_texThickness.Sample(g_samLinear, uv).r;
    output.orm.a = g_overrideMaterial ? 0.0f : (g_materialSubsurfaceStrength * thicknessScale);

    output.subsurface = float4(g_materialSubsurfaceColor, 1.0f);
    
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
    
    output.normal = float4(EncodeNormal(n), 1.0f);
    
    return output;
}