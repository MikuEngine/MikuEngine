#include "../Include/Shared.hlsli"

PS_OUTPUT_GBUFFER main(PS_INPUT_GBUFFER input)
{
    PS_OUTPUT_GBUFFER output = (PS_OUTPUT_GBUFFER) 0;
    
    output.baseColor = g_texBaseColor.Sample(g_samLinear, input.texCoord);    
    output.baseColor = float4(pow(abs(output.baseColor.rgb), 2.2f), 1.0f);
    
    float3 encodedNormal = g_texNormal.Sample(g_samLinear, input.texCoord).rgb;
    output.emissive = float4(pow(abs(g_texEmissive.Sample(g_samLinear, input.texCoord).rgb), 2.2f), 1.0f);
    output.orm.r = g_texAmbientOcclusion.Sample(g_samLinear, input.texCoord).r;
    output.orm.g = g_texRoughness.Sample(g_samLinear, input.texCoord).r;
    output.orm.b = g_texMetalness.Sample(g_samLinear, input.texCoord).r;
    output.orm.a = 1.0f;
    
    output.baseColor *= g_materialBaseColor;
    output.emissive.rgb = output.emissive.rgb * g_materialEmissive * g_materialEmissiveIntensity;
    
    if (g_overrideMaterial)
    {
        output.baseColor = g_materialBaseColor;
        output.emissive = 0.0f;
        output.orm.r = g_materialAmbientOcclusion;
        output.orm.g = g_materialRoughness;
        output.orm.b = g_materialMetalness;
    }
    
    // normal
    float3x3 tbn = float3x3(
        normalize(input.tangent),
        normalize(input.binormal),
        normalize(input.normal)
    );
    
    float3 n = normalize(mul(DecodeNormal(encodedNormal), tbn));
    output.normal = float4(EncodeNormal(n), 1.0f);
    
    return output;
}