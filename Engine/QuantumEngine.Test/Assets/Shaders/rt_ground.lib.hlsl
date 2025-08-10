#include "RTStructs.hlsli"
#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"

cbuffer MissProps : register(b0, space1)
{
    uint missIndex;
    float3 h; //dummy field to keep the register multiple of 4
};

cbuffer ObjectTransformData : register(b1, space1)
{
    TransformData transformData;
};

cbuffer CameraData : register(b2, space1)
{
    CameraData cameraData;
};

cbuffer LightData : register(b3, space1)
{
    LightData lightData;
}

RaytracingAccelerationStructure gRtScene : register(t3, space1);

Texture2D mainTexture : register(t0, space1);
sampler mainSampler : register(s0, space1);

StructuredBuffer<uint> g_indices : register(t1, space1);
StructuredBuffer<Vertex> g_vertices : register(t2, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint baseIndex = PrimitiveIndex() * 3;
    Vertex v1 = g_vertices[g_indices[baseIndex]];
    Vertex v2 = g_vertices[g_indices[baseIndex + 1]];
    Vertex v3 = g_vertices[g_indices[baseIndex + 2]];
    float3 rayDirection = WorldRayDirection();
    float3 normal = v1.normal + attribs.barycentrics.x * (v2.normal - v1.normal) + attribs.barycentrics.y * (v3.normal - v1.normal);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float2 uv = v1.uv + attribs.barycentrics.x * (v2.uv - v1.uv) + attribs.barycentrics.y * (v3.uv - v1.uv);
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    float shadowValue = 1.0f;
    
    for (uint i = 0; i < lightData.directionalLightCount; i++)
    {
        GeneralPayload innerPayload;
        innerPayload.targetMode = 2;
        innerPayload.recursionCount = payload.recursionCount + 1;
        RayDesc ray;
        ray.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        ray.Direction = -lightData.directionalLights[i].direction;
        ray.TMin = 0.1;
        ray.TMax = 100000;
        TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, missIndex, ray, innerPayload);

        if(innerPayload.hit > 0)
            shadowValue = max(shadowValue - 0.7f, 0);
    }
    
    for (uint i = 0; i < lightData.pointLightCount; i++)
    {
        float d = distance(lightData.pointLights[i].position, WorldRayOrigin() + RayTCurrent() * WorldRayDirection());
        if (d > lightData.pointLights[i].radius)
            continue;
        
        GeneralPayload innerPayload;
        innerPayload.targetMode = 2;
        innerPayload.recursionCount = payload.recursionCount + 1;
        
        RayDesc ray;
        ray.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        ray.Direction = normalize(lightData.pointLights[i].position - ray.Origin);
        ray.TMin = 0.1;
        ray.TMax = min(lightData.pointLights[i].radius, d);
        TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, missIndex, ray, innerPayload);

        if (innerPayload.hit > 0.01f)
            shadowValue = max(shadowValue - innerPayload.hit, 0);
    }
    
    payload.color = shadowValue * (texColor.xyz);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.hit = 0;
}