#include "../Include/Shared.hlsli"

PS_INPUT_TEXCOORD main(VS_INPUT_TEXCOORD input)
{
    PS_INPUT_TEXCOORD output = (PS_INPUT_TEXCOORD) 0;
    
    float2 shift = float2(-(g_pivot.x - 0.5f), g_pivot.y - 0.5f);
    float4 position = float4(input.position, 1.0f);
    position.xy += shift;
    
    output.position = mul(position, g_world);
    output.position = mul(output.position, g_viewProjection);
    
    output.texCoord = input.texCoord * g_uvScale + g_uvOffset;
    
    return output;
}