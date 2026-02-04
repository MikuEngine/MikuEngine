#include "../Include/Shared.hlsli"

// Spot light shadow: same as directional but use spot light view-projection from ShadowSpot CB
PS_INPUT_TEXCOORD main(VS_INPUT_COMMON input)
{
    PS_INPUT_TEXCOORD output = (PS_INPUT_TEXCOORD) 0;

    output.position = mul(float4(input.position, 1.0f), g_world);
    output.position = mul(output.position, g_spotShadowDepthViewProj);

    output.texCoord = input.texCoord;

    return output;
}
