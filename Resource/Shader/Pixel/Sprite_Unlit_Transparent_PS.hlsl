#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_TEXCOORD input) : SV_Target
{
    return g_texBaseColor.Sample(g_samLinear, input.texCoord);
}