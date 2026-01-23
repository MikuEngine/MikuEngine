#include "../Include/Shared.hlsli"

GS_INPUT_POSITION main(VS_INPUT_TEXCOORD input, uint instanceID : SV_InstanceID)
{
    GS_INPUT_POSITION output;
    
    output.worldPos = mul(float4(input.position, 1.0f), g_world);
    output.position = mul(output.worldPos, g_viewProjections[instanceID]);
    output.rtIndex = (g_shadowLightIndex * 6) + instanceID;
    output.texCoord = input.texCoord;
    
    return output;
}