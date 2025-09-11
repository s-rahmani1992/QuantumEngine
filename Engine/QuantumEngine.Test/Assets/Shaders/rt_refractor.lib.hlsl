#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer ObjectTransformData : register(b2)
{
    TransformData transformData;
};

cbuffer MaterialProps : register(b0, space1)
{
    float4 color;
    uint castShadow;
    float refractionFactor;
    float2 dummy;
};

cbuffer MissProps : register(b1, space1)
{
    uint missIndex;
    float3 h; //dummy field to keep the register multiple of 4
};

RaytracingAccelerationStructure gRtScene : register(t3, space1);

StructuredBuffer<uint> g_indices : register(t1, space1);
StructuredBuffer<Vertex> g_vertices : register(t2, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2) // It's for shadow
    {
        payload.hit = castShadow > 0 ? 0.7f : 0.0f;
        return;
    }
    
    uint baseIndex = PrimitiveIndex() * 3;
    Vertex v1 = g_vertices[g_indices[baseIndex]];
    Vertex v2 = g_vertices[g_indices[baseIndex + 1]];
    Vertex v3 = g_vertices[g_indices[baseIndex + 2]];
    float3 rayDirection = normalize(WorldRayDirection());
    float3 normal = v1.normal + attribs.barycentrics.x * (v2.normal - v1.normal) + attribs.barycentrics.y * (v3.normal - v1.normal);
    normal = normalize(mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz);
    
    if (payload.recursionCount <= 4)
    {
        RayDesc ray;
        ray.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        
        float cosI = dot(normal, rayDirection);
        float refractionIndex = 1.0f;
        
        if (cosI < 0) // from outside to inside
        {
            refractionIndex = 1.0f / refractionFactor;
        }
        else // from inside to outside
        {
            refractionIndex = refractionFactor;
        }
        
        float sinT2 = refractionIndex * refractionIndex * (1.0 - cosI * cosI);
        
        if(sinT2 > 1.0f) // Total internal reflection
        {
            ray.Direction = normalize(rayDirection - 2 * cosI * normal);
        }
        else
        {
            ray.Direction = normalize(refractionIndex * rayDirection + (refractionIndex * cosI - sqrt(1 - sinT2)) * normal);
        }
        
        ray.TMin = 0.1;
        ray.TMax = 100000;
        
        GeneralPayload innerPayload;
        innerPayload.targetMode = 1;
        innerPayload.recursionCount = payload.recursionCount + 1;
        TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, missIndex, ray, innerPayload);
        payload.color = innerPayload.color.xyz;
    }
    else
    {
        payload.color = float3(1.0f, 1.0f, 1.0f);
    }
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.4, 0.6, 0.2);
}