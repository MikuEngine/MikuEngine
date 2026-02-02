#include "../Include/Shared.hlsli"

PS_INPUT_GBUFFER main(VS_INPUT_SKINNING input)
{
    PS_INPUT_GBUFFER output = (PS_INPUT_GBUFFER) 0;
        
    float4x4 weightedOffsetPose;
    weightedOffsetPose = mul(input.blendWeights.x, g_boneTransform[input.blendIndices.x]);
    weightedOffsetPose += mul(input.blendWeights.y, g_boneTransform[input.blendIndices.y]);
    weightedOffsetPose += mul(input.blendWeights.z, g_boneTransform[input.blendIndices.z]);
    weightedOffsetPose += mul(input.blendWeights.w, g_boneTransform[input.blendIndices.w]);
    
    float4x4 world = mul(weightedOffsetPose, g_world);
    
    output.position = mul(float4(input.position, 1.0f), world);
    output.worldPosition = output.position.xyz;
    output.position = mul(output.position, g_viewProjection);
    
    output.normal = mul(input.normal, (float3x3) world);
    output.tangent = mul(input.tangent, (float3x3) world);
    output.binormal = mul(input.binormal, (float3x3) world);
    
    if (input.color.r >= 0.5f && input.color.g <= 0.5f && input.color.b <= 0.5f)
    {
        float offset = 0.591f - 0.447f;
        
        float2 rotUV = float2(0.755f - input.texCoord.y + offset * 2, input.texCoord.x - 0.188f);
        
        output.texCoord = rotUV + float2(0.447f, 0.0f);
    }
    else
    {
        output.texCoord = input.texCoord;
    }
    
    return output;
}