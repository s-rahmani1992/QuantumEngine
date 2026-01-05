#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
    float3 tangent : TANGENT;
};

struct GS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
    float3 worldPos : POSITION;
};

cbuffer _ObjectTransformData : register(b0)
{
    TransformData transformData;
};

cbuffer _CameraData : register(b1)
{
    CameraData cameraData;
};


cbuffer _LightData : register(b2)
{
    LightData lightData;
}


cbuffer _CurveProperties : register(b3)
{
    float _width;
};

cbuffer MaterialProps : register(b4)
{
    float4 color;
    float ambient;
    float diffuse;
    float specular;
};

VS_INPUT vs_main(VS_INPUT vertexIn)
{
    return vertexIn;
}

[maxvertexcount(4)]
void gs_main(line VS_INPUT vertexIn[2], inout TriangleStream<GS_OUTPUT> triStream)
{
    float4x4 mat = mul(transformData.modelViewMatrix, cameraData.projectionMatrix);
    GS_OUTPUT gsOut[4];
    float3 edge1 = cross(vertexIn[0].norm, vertexIn[0].tangent);
    gsOut[0].worldPos = vertexIn[0].pos + _width * edge1;
    gsOut[0].texCoord = float2(vertexIn[0].texCoord.x, 1);
    gsOut[0].norm = vertexIn[0].norm;
    gsOut[0].pos = mul(float4(gsOut[0].worldPos, 1.0f), mat);
    gsOut[1].worldPos = vertexIn[0].pos - _width * edge1;
    gsOut[1].texCoord = float2(vertexIn[0].texCoord.x, 0);
    gsOut[1].norm = vertexIn[0].norm;
    gsOut[1].pos = mul(float4(gsOut[1].worldPos, 1.0f), mat);
    
    float3 edge2 = cross(vertexIn[1].norm, vertexIn[1].tangent);
    gsOut[2].worldPos = vertexIn[1].pos + _width * edge2;
    gsOut[2].texCoord = float2(vertexIn[1].texCoord.x, 1);
    gsOut[2].norm = vertexIn[1].norm;
    gsOut[2].pos = mul(float4(gsOut[2].worldPos, 1.0f), mat);
    gsOut[3].worldPos = vertexIn[1].pos - _width * edge2;
    gsOut[3].texCoord = float2(vertexIn[1].texCoord.x, 0);
    gsOut[3].norm = vertexIn[1].norm;
    gsOut[3].pos = mul(float4(gsOut[3].worldPos, 1.0f), mat);
    
    triStream.Append(gsOut[0]);
    triStream.Append(gsOut[1]);
    triStream.Append(gsOut[2]);
    triStream.Append(gsOut[3]);
}

float4 ps_main(GS_OUTPUT input) : SV_TARGET
{
    float3 ads = float3(ambient, diffuse, specular);
  
    float3 lightFactor = PhongLight(lightData, cameraData.position, input.worldPos, input.norm, ads);
    
    float4 texColor = color;
    return float4(lightFactor * texColor.xyz, 1);
}