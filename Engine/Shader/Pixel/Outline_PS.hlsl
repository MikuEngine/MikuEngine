#include "../Include/Shared.hlsli"

float4 main(PS_INPUT input) : SV_Target
{
    int thickness = 5;
    
    uint centerVal = g_texStencilMap.Load(int3(input.position.xy, 0)).g;
    
    if (centerVal > 0)
    {
        discard;
    }
    
    for (int y = -thickness; y <= thickness; ++y)
    {
        for (int x = -thickness; x <= thickness; ++x)
        {
            if (length(float2(x, y)) > thickness)
            {
                continue;
            }
            
            uint neighborVal = g_texStencilMap.Load(int3(input.position.xy + int2(x, y), 0)).g;

            if (neighborVal > 0)
            {
                return float4(1.0f, 0.5f, 0.0f, 1.0f);
            }
        }
    }
    
    discard;
    
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}