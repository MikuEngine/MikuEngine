#include "../Include/Shared.hlsli"

[maxvertexcount(3)]
void main(triangle GS_INPUT_POSITION input[3], inout TriangleStream<PS_INPUT_RT_INDEX> outputStream)
{
    PS_INPUT_RT_INDEX output;
    
    for (int v = 0; v < 3; ++v)
    {
        output.worldPos = input[v].worldPos; // World Pos À¯Áö
        output.texCoord = input[v].texCoord;
        output.position = input[v].position;
        output.rtIndex = input[v].rtIndex;
        
        outputStream.Append(output);
    }
    
    outputStream.RestartStrip();
}