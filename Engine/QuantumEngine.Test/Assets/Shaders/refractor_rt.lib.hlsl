#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer ObjectTransformData : register(b2, space1)
{
    TransformData transformData;
};

cbuffer MaterialProps : register(b0, space1)
{
    uint missIndex;
    float refractionFactor;
    uint maxRecursion;
    float dummy;
};

RaytracingAccelerationStructure gRtScene : register(t3, space1);
StructuredBuffer<uint> g_indices : register(t1, space1);
StructuredBuffer<Vertex> g_vertices : register(t2, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 rayDirection = normalize(WorldRayDirection());
    float3 normal = CalculateNormal(g_indices, g_vertices, attribs.barycentrics);
    normal = normalize(mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz);
    
    if (payload.recursionCount <= maxRecursion)
    {
        bool isReflect;
        RayDesc ray;
        ray.Origin = CalculeteHitPosition();
        ray.Direction = Refract(rayDirection, normal, refractionFactor, isReflect);
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
    payload.color = float3(0.1f, 0.7f, 0.3f);
}