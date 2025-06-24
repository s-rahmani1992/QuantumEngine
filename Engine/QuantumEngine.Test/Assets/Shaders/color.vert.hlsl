
cbuffer Transform : register(b0)
{
    float scale;
    float2 offset;
};

float4 main(float3 pos : POSITION) : SV_POSITION
{
    float2 p = (scale * pos.xy) + offset;
    return float4(p.xy, pos.z, 1.0f);
}