#include "../Include/Shared.hlsli"

GS_INPUT_POSITION main(VS_INPUT_COMMON input, uint instanceID : SV_InstanceID)
{
    GS_INPUT_POSITION output = (GS_INPUT_POSITION) 0;
    
    float4x4 world = mul(g_boneTransform[g_boneIndex], g_world);
    
    output.worldPos = mul(float4(input.position, 1.0f), world);
    output.position = mul(output.worldPos, g_viewProjections[instanceID]);
    output.rtIndex = (g_shadowLightIndex * 6) + instanceID;
    output.texCoord = input.texCoord;
    
    return output;
}