#include "../Include/Shared.hlsli"

PS_INPUT main(VS_INPUT_COMMON input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    output.pos = mul(float4(input.pos, 1.0f), g_world);
    output.pos = mul(output.pos, g_lightView);
    output.pos = mul(output.pos, g_lightProjection);
    
    output.tex = input.tex;
    
    return output;
}