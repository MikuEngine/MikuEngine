#include "../Include/Shared.hlsli"

void main(PS_INPUT input)
{
    float4 texOpacColor = g_texOpacity.Sample(g_samLinear, input.tex);
    
    clip(texOpacColor.a - 0.5f);
}