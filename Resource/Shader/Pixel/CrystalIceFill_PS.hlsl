#include "../Include/Shared.hlsli"
#include "../Include/PBR_Shared.hlsli"

// 면(normal) 단위로 채우기: 같은 노말 = 한 번에 채워짐, 아래 향한 면부터 올라감
float4 main(PS_INPUT_ICEFILL input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    // ─── 얼음 채우기: 면 단위 — 같은 normal 면이 함께 채워지고, 아래 면부터 올라감 ───
    if (g_iceFillEnable)
    {
        float3 nFace = normalize(input.normal);
        if (!isFrontFace)
            nFace = -nFace;
        // 노말을 격자로 양자화 → 같은 면(같은 normal)이 같은 그룹
        float3 nQ = floor(nFace * 6.0f + 0.5f) / 4.0f;
        float len = length(nQ);
        if (len < 1e-4f)
            nQ = float3(0.0f, 1.0f, 0.0f);
        else
            nQ /= len;
        // y = -1(아래) → fillOrder 0 먼저, y = +1(위) → fillOrder 1 나중
        float fillOrder = saturate((nQ.y + 1.0f) * 0.5f);
        if (g_iceFillAmount < fillOrder)
            clip(-1);
    }

    // ─── 이하 LightTransparent_PS와 동일 (PBR 라이팅) ───
    float2 uv = input.texCoord;
    float4 baseColor = g_texBaseColor.Sample(g_samLinear, uv);
    baseColor = float4(pow(abs(baseColor.rgb), 2.2f), baseColor.a);
    float3 encodedNormal = g_texNormal.Sample(g_samLinear, uv).rgb;
    float3 worldPosition = input.worldPosition;
    float3 emissive = pow(abs(g_texEmissive.Sample(g_samLinear, uv).rgb), 2.2f);
    float ao = g_texAmbientOcclusion.Sample(g_samLinear, uv).r;
    float roughness = g_texRoughness.Sample(g_samLinear, uv).r;
    float metalness = g_texMetalness.Sample(g_samLinear, uv).r;

    baseColor *= g_materialBaseColor;
    baseColor.a *= g_materialAlpha;
    emissive = emissive * g_materialEmissive * g_materialEmissiveIntensity;

    if (g_overrideMaterial)
    {
        baseColor = g_materialBaseColor;
        baseColor.a *= g_materialAlpha;
        emissive = 0.0f;
        ao = g_materialAmbientOcclusion;
        roughness = g_materialRoughness;
        metalness = g_materialMetalness;
    }
    else
    {
        roughness = roughness * g_materialRoughness;
        metalness = saturate(metalness + g_materialMetalness);
        ao = ao * g_materialAmbientOcclusion;
    }

    float3x3 tbn = float3x3(
        normalize(input.tangent),
        normalize(input.binormal),
        normalize(input.normal)
    );
    float3 n = normalize(mul(DecodeNormal(encodedNormal), tbn));
    if (!isFrontFace)
        n = -n;

    float3 v = normalize(g_cameraWorldPosition - worldPosition);
    float3 l = -normalize(g_mainLightWorldDirection);
    float3 h = normalize(l + v);
    float nDotH = saturate(dot(n, h));
    float nDotL = saturate(dot(n, l));
    float nDotV = saturate(dot(n, v));
    float hDotV = saturate(dot(h, v));
    float shadowFactor = 1.0f;

    float3 f0 = lerp(DielectricFactor, baseColor.rgb, metalness);
    float g = GAFSchlickGGX(nDotV, nDotL, roughness);

    float3 directLighting = 0.0f;
    {
        float d = NDFGGXTR(nDotH, max(0.1f, roughness));
        float3 f = FresnelSchlick(f0, hDotV);
        float3 kd = lerp(1.0f - f, 0.0f, metalness);
        float3 diffuseBRDF = kd * baseColor.rgb / PI;
        float3 specularBRDF = (f * d * g) / max(EPSILON, 4.0f * nDotL * nDotV);
        directLighting = (diffuseBRDF + specularBRDF) * g_mainLightColor * g_mainLightIntensity * nDotL * shadowFactor;
    }

    float3 ambientLighting = 0.0f;
    if (g_useIBL)
    {
        float3 f = FresnelSchlick(f0, nDotV);
        float3 kd = lerp(1.0f - f, 0.0f, metalness);
        float3 irradiance = g_texIBLIrradiance.Sample(g_samLinear, n).rgb;
        float3 diffuseIBL = kd * baseColor.rgb / PI * irradiance;
        uint specularTextureLevels, width, height;
        g_texIBLSpecular.GetDimensions(0, width, height, specularTextureLevels);
        float3 viewReflect = -(v - 2.0f * nDotV * n);
        float3 prefilteredColor = g_texIBLSpecular.SampleLevel(g_samLinear, viewReflect, roughness * specularTextureLevels).rgb;
        float2 specularBRDF = g_texIBLSpecularBRDFLUT.Sample(g_samClamp, float2(nDotV, roughness)).rg;
        float3 specularIBL = prefilteredColor * (f0 * specularBRDF.x + specularBRDF.y);
        ambientLighting = (diffuseIBL + specularIBL) * ao;
    }

    float3 final = directLighting + ambientLighting + emissive;
    return float4(final, baseColor.a);
}
