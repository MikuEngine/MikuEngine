#include "../Include/Shared.hlsli"

void main(PS_INPUT_TEXCOORD input)
{
    float threshold = 0.001f;
    
    clip(g_texBaseColor.Sample(g_samLinear, input.texCoord).a - threshold);
}