#include "../Include/Shared.hlsli"

PS_INPUT_WORLD_POSITION main(VS_INPUT_POSITION input)
{
    PS_INPUT_WORLD_POSITION output = (PS_INPUT_WORLD_POSITION) 0;
    
    output.position = mul(float4(input.position, 1.0f), g_world);
    output.worldPosition = output.position.xyz;
    output.position = mul(output.position, g_viewProjection);
    
    return output;
}