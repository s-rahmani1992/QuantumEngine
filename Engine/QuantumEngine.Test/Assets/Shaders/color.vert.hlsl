
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

struct TransformData {
    float4x4 modelMatrix;
    float4x4 rotationMatrix;
    float4x4 modelViewMatrix;
};          

cbuffer ObjectTransformData : register(b0)
{
    TransformData transformData;
};

cbuffer viewTransform : register(b1)
{
    float4x4 viewMatrix;
    float4x4 projectMatrix;
};

VS_OUTPUT main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, projectMatrix));
    vsOut.texCoord = vertexIn.texCoord;
    return vsOut;
}