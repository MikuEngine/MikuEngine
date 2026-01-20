#include "../Include/Shared.hlsli"

GS_INPUT_POSITION main(VS_INPUT_TEXCOORD input)
{
    GS_INPUT_POSITION output;
    
    output.position = mul(float4(input.position, 1.0f), g_world);
    output.texCoord = input.texCoord;
    
    return output;
}