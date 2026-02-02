#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_PARTICLE input) : SV_TARGET
{
    float4 texColor = g_texBaseColor.Sample(g_samLinear, input.texCoord);
    float4 base = texColor * input.color;
    float3 emissive = input.emissiveColor * input.emissiveIntensity;
    return float4(base.rgb * emissive, base.a);
}