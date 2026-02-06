#include "../Include/Shared.hlsli"

// Static_VS와 동일 (면 단위 채우기용 normal 등만 전달)
PS_INPUT_ICEFILL main(VS_INPUT_COMMON input)
{
    PS_INPUT_ICEFILL output = (PS_INPUT_ICEFILL) 0;

    output.position = mul(float4(input.position, 1.0f), g_world);
    output.worldPosition = output.position.xyz;
    output.position = mul(output.position, g_viewProjection);

    output.normal = mul(input.normal, (float3x3) g_worldInverseTranspose);
    output.tangent = mul(input.tangent, (float3x3) g_worldInverseTranspose);
    output.binormal = mul(input.binormal, (float3x3) g_worldInverseTranspose);

    output.texCoord = input.texCoord;

    return output;
}
