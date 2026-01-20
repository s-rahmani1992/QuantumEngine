#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"
#include "LightStructs.hlsli"

cbuffer _ObjectTransformData : register(b0, space1)
{
    TransformData transformData;
};

cbuffer MaterialProps : register(b4, space1)
{
    uint castShadow = 0;
    float reflectivity;
    float ambient;
    float diffuse;
    float specular;
};

cbuffer _CameraData : register(b1, space1)
{
    CameraData cameraData;
};

cbuffer _LightData : register(b2, space1)
{
    LightData lightData;
}

cbuffer _RTProperties : register(b3, space1)
{
    uint _missIndex;
};

RaytracingAccelerationStructure _RTScene : register(t3, space1);

Texture2D mainTexture : register(t0, space1);
sampler mainSampler : register(s0, space1);

StructuredBuffer<uint> _indexBuffer : register(t1, space1);
StructuredBuffer<Vertex> _vertexBuffer : register(t2, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2 && castShadow > 0)
    {
        payload.hit = 1;
        return;
    }
    
    payload.hit = 1;
    
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, position, normal, ads);
    
    if (payload.recursionCount <= 3)
    {
        float3 rayDirection = WorldRayDirection();
        
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = normalize(reflect(rayDirection, normal));
        ray.TMin = 0.1;
        ray.TMax = 100000;
        
        GeneralPayload innerPayload;
        innerPayload.targetMode = 1;
        innerPayload.recursionCount = payload.recursionCount + 1;
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, innerPayload);
        
        if(innerPayload.hit == 0)
            payload.color = lightFactor * texColor.xyz;
        else
            payload.color = lightFactor * (reflectivity * innerPayload.color.xyz + (1 - reflectivity) * texColor.xyz);
    }
    else
        payload.color = lightFactor * texColor.xyz;
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.0, 0.0, 0.0);
    payload.hit = 0;
}