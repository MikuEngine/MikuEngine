#include "../Include/Shared.hlsli"

PS_INPUT_TEXCOORD main(VS_INPUT_COMMON input)
{
    PS_INPUT_TEXCOORD output = (PS_INPUT_TEXCOORD) 0;
    
    output.position = mul(float4(input.position, 1.0f), g_world);
    output.position = mul(output.position, g_viewProjection);
    output.texCoord = input.texCoord;
    
    return output;
}