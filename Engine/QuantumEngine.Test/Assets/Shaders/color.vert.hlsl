
struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer Transform : register(b0)
{
    float scale;
    float2 offset;
};

VS_OUTPUT main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    float2 p = (scale * vertexIn.pos.xy) + offset;
    vsOut.pos = float4(p.xy, vertexIn.pos.z, 1.0f);
    vsOut.texCoord = vertexIn.texCoord;
    return vsOut;
}