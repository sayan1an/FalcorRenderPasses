layout(binding = 0) SamplerState smSampler : register(s0);
layout(binding = 1) texture2D shadowMap : register(t0);

float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    float depth = shadowMap.Sample(smSampler, texC).x;//shadowMap[10, 10].x;

    return float4(depth, 1-depth, 0, 1);
}
