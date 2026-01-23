#include "../Include/Shared.hlsli"

GS_INPUT_POSITION main(VS_INPUT_SKINNING input, uint instanceID : SV_InstanceID)
{
    GS_INPUT_POSITION output = (GS_INPUT_POSITION) 0;
        
    float4x4 weightedOffsetPose;
    weightedOffsetPose = mul(input.blendWeights.x, g_boneTransform[input.blendIndices.x]);
    weightedOffsetPose += mul(input.blendWeights.y, g_boneTransform[input.blendIndices.y]);
    weightedOffsetPose += mul(input.blendWeights.z, g_boneTransform[input.blendIndices.z]);
    weightedOffsetPose += mul(input.blendWeights.w, g_boneTransform[input.blendIndices.w]);
    
    float4x4 world = mul(weightedOffsetPose, g_world);
    
    output.worldPos = mul(float4(input.position, 1.0f), world);
    output.position = mul(output.worldPos, g_viewProjections[instanceID]);
    output.rtIndex = (g_shadowLightIndex * 6) + instanceID;
    output.texCoord = input.texCoord;
    
    return output;
}