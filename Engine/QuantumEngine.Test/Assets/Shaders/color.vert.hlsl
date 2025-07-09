
struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer wTransform : register(b0)
{
    float4x4 worldMatrix;
    float4x4 projectMatrix;
};

cbuffer viewTransform : register(b1)
{
    float4x4 viewMatrix;
};

VS_OUTPUT main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(mul(worldMatrix, viewMatrix), projectMatrix));
    vsOut.texCoord = vertexIn.texCoord;
    return vsOut;
}