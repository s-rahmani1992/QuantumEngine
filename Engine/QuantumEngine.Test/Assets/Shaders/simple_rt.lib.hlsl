#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer _ObjectTransformData : register(b1, space1)
{
    TransformData transformData;
};

cbuffer _CameraData : register(b2, space1)
{
    CameraData cameraData;
};

Texture2D mainTexture : register(t0, space1);
sampler mainSampler : register(s0, space1);

StructuredBuffer<uint> _indexBuffer : register(t1, space1);
StructuredBuffer<Vertex> _vertexBuffer : register(t2, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    
    payload.color = texColor.xyz;
    
    if(payload.targetMode == 1)
        payload.hit = 1;
    else if (payload.targetMode == 2)
        payload.hit = 0;
}