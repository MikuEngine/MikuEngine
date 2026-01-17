#include "../Include/Shared.hlsli"

uint main(PS_INPUT_TEXCOORD input) : SV_Target
{
    float a = g_texBaseColor.Sample(g_samLinear, input.texCoord).a;
	
    clip(a - 0.1f);
	
	return g_pickingId;
}