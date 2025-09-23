#include "TransformStructs.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float2 normPos : POSITION;
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
    float4 position = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vsOut.pos = position;
    vsOut.texCoord = vertexIn.texCoord;
    float2 ndc = position.xy;
    vsOut.normPos = float2(ndc.x, -ndc.y) * 0.5f + 0.5f;
    return vsOut;
}