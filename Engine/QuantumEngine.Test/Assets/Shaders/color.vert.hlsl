#include "TransformStructs.hlsli"

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

cbuffer ObjectTransformData : register(b0)
{
    TransformData transformData;
};

cbuffer CameraData : register(b1)
{
    CameraData cameraData;
};

VS_OUTPUT main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vsOut.texCoord = vertexIn.texCoord;
    return vsOut;
}