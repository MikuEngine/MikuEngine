#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_PARTICLE input) : SV_TARGET
{
    float4 texColor = g_texBaseColor.Sample(g_samLinear, input.texCoord);
    
    return texColor * input.color;
}