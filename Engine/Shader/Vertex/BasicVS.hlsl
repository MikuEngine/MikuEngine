#include "../Include/Shared.hlsli"

PS_INPUT_SHADOW main(VS_INPUT_COMMON input)
{
    PS_INPUT_SHADOW output = (PS_INPUT_SHADOW) 0;
    
    output.pos = mul(float4(input.pos, 1.0f), g_world);
    output.worldPos = output.pos.xyz;
    output.pos = mul(output.pos, g_view);
    output.pos = mul(output.pos, g_projection);
    
    //output.norm = mul(input.norm, (float3x3) g_world);
    //output.tan = mul(input.tan, (float3x3) g_world);
    //output.binorm = mul(input.binorm, (float3x3) g_world);
    
    //output.tex = input.tex;
    
    //output.lightViewPos = mul(float4(output.worldPos, 1.0f), g_lightView);
    //output.lightViewPos = mul(output.lightViewPos, g_lightProjection);
    
    return output;
}