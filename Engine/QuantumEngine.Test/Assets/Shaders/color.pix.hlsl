
struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer Material : register(b2)
{
    float4 color;
};

Texture2D mainTexture : register(t0);
sampler mainSampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    return color * texColor;
}