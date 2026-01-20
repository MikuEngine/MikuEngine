#include "../Include/Shared.hlsli"

[maxvertexcount(18)]
void main(triangle GS_INPUT_POSITION input[3], inout TriangleStream<PS_INPUT_RT_INDEX> outputStream)
{
    for (int face = 0; face < 6; ++face)
    {
        PS_INPUT_RT_INDEX output;
        output.rtIndex = (g_shadowLightIndex * 6) + face;
        for (int v = 0; v < 3; ++v)
        {
            output.worldPos = input[v].position; // World Pos À¯Áö
            output.texCoord = input[v].texCoord;
            output.position = mul(input[v].position, g_viewProjections[face]);
            outputStream.Append(output);
        }
        outputStream.RestartStrip();
    }
}