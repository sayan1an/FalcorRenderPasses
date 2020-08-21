layout(binding = 0) SamplerState smSampler : register(s0);
layout(binding = 1) texture2D shadowMap : register(t0);
layout(binding = 2) texture2D worldPos : register(t1);
layout(binding = 3) cbuffer LightVP : register(b0)
{
    float4x4 lightVP;
}

float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    float4 wPos = worldPos.Sample(smSampler, texC);
    wPos.w = 1.0;
    float4 cPosLight = mul(wPos, lightVP);
    cPosLight /= cPosLight.w;

    cPosLight.xy = cPosLight.xy * 0.5 + 0.5;
    cPosLight.y = 1 -  cPosLight.y;
    float depth = shadowMap.Sample(smSampler, cPosLight.xy).x;//shadowMap[10, 10].x);

    if (depth < cPosLight.z)
        return float4(0.0);

    return float4(1,1,1,1);
}
