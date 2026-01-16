#include "../Include/Shared.hlsli"

PS_INPUT_TEXCOORD main(VS_INPUT_TEXCOORD input)
{
    PS_INPUT_TEXCOORD output = (PS_INPUT_TEXCOORD) 0;
	
    float4 p = mul(float4(input.position.xyz, 1.0f), g_world);
    
    float2 ndc;
    ndc.x = (p.x / g_screenSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (p.y / g_screenSize.y) * 2.0f;

    output.position = float4(ndc, 0.0f, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}