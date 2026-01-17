#include "../Include/Shared.hlsli"

float4 main(PS_INPUT input) : SV_Target
{
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    bool shouldDiscard = true;
    
    uint centerVal = g_texStencilMap.Load(int3(input.position.xy, 0)).g;
    
    if (centerVal == 0)
    {
        int thickness = 5;
        int stride = 2;

        [unroll]
        for (int y = -thickness; y <= thickness; y += stride)
        {
            [unroll]
            for (int x = -thickness; x <= thickness; x += stride)
            {
                if (x == 0 && y == 0)
                {
                    continue;
                }

                // 원형 범위 체크
                if ((x * x + y * y) > (thickness * thickness))
                {
                    continue;
                }
                
                uint neighborVal = g_texStencilMap.Load(int3(input.position.xy + int2(x, y), 0)).g;

                if (neighborVal > 0)
                {
                    finalColor = float4(1.0f, 0.5f, 0.0f, 1.0f);
                    shouldDiscard = false;
                    break;
                }
            }
            
            if (!shouldDiscard)
            {
                break;
            }
        }
    }
    
    if (shouldDiscard)
    {
        discard;
    }
    
    return finalColor;
}