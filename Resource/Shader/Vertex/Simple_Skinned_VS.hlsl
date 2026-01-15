#include "../Include/Shared.hlsli"

PS_INPUT_TEXCOORD main(VS_INPUT_SKINNING input)
{
    PS_INPUT_TEXCOORD output = (PS_INPUT_TEXCOORD) 0;
        
    float4x4 weightedOffsetPose;
    weightedOffsetPose = mul(input.blendWeights.x, g_boneTransform[input.blendIndices.x]);
    weightedOffsetPose += mul(input.blendWeights.y, g_boneTransform[input.blendIndices.y]);
    weightedOffsetPose += mul(input.blendWeights.z, g_boneTransform[input.blendIndices.z]);
    weightedOffsetPose += mul(input.blendWeights.w, g_boneTransform[input.blendIndices.w]);
    
    float4x4 world = mul(weightedOffsetPose, g_world);
    
    output.position = mul(float4(input.position, 1.0f), world);
    output.position = mul(output.position, g_viewProjection);
    
    output.texCoord = input.texCoord;
    
    return output;
}