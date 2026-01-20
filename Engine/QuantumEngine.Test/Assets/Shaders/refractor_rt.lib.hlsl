#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer _ObjectTransformData : register(b2, space1)
{
    TransformData transformData;
};

cbuffer _RTProperties : register(b1, space1)
{
    uint _missIndex;
};

cbuffer MaterialProps : register(b0, space1)
{
    float refractionFactor;
    uint maxRecursion;
};

RaytracingAccelerationStructure _RTScene : register(t3, space1);
StructuredBuffer<uint> _indexBuffer : register(t1, space1);
StructuredBuffer<Vertex> _vertexBuffer : register(t2, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2)
    {
        payload.hit = 0;
        return;
    }
    
    float3 rayDirection = normalize(WorldRayDirection());
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
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
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, innerPayload);
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