#include "../Include/Shared.hlsli"

PS_INPUT_LOCAL_POSITION main(VS_INPUT_POSITION input)
{
    PS_INPUT_LOCAL_POSITION output = (PS_INPUT_LOCAL_POSITION) 0;
    
    float4x4 viewRotation = g_view;
    viewRotation._41_42_43 = 0.0f;
    
    output.position = mul(float4(input.position, 0.0f), viewRotation);
    output.position = mul(output.position, g_projection).xyww;
    
    output.localPosition = input.position;
    
    return output;
}