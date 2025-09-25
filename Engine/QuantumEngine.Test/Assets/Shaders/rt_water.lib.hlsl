#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"
#include "LightStructs.hlsli"

cbuffer ObjectTransformData : register(b4)
{
    TransformData transformData;
};

cbuffer MaterialProps : register(b0, space1)
{
    float4 color;
    uint castShadow;
    float3 dummy;
};

cbuffer MissProps : register(b1, space1)
{
    uint missIndex;
    float3 h; //dummy field to keep the register multiple of 4
};

cbuffer CameraData : register(b2)
{
    CameraData cameraData;
};

cbuffer LightData : register(b3, space0)
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
    if (payload.targetMode == 2) // It's for shadow
    {
        payload.hit = castShadow > 0 ? 0.7f : 0.0f;
        return;
    }
    
    uint baseIndex = PrimitiveIndex() * 3;
    Vertex v1 = g_vertices[g_indices[baseIndex]];
    Vertex v2 = g_vertices[g_indices[baseIndex + 1]];
    Vertex v3 = g_vertices[g_indices[baseIndex + 2]];
    float3 rayDirection = WorldRayDirection();
    float3 normal = v1.normal + attribs.barycentrics.x * (v2.normal - v1.normal) + attribs.barycentrics.y * (v3.normal - v1.normal);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float2 uv = v1.uv + attribs.barycentrics.x * (v2.uv - v1.uv) + attribs.barycentrics.y * (v3.uv - v1.uv);
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    
    if (payload.recursionCount <= 2)
    {
        RayDesc ray;
        ray.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        ray.Direction = rayDirection - 2 * dot(normal, rayDirection) * normal;

        ray.TMin = 0.1;
        ray.TMax = 100000;
        
        GeneralPayload innerPayload;
        innerPayload.targetMode = 1;
        innerPayload.recursionCount = payload.recursionCount + 1;
        TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, missIndex, ray, innerPayload);
        
        float3 lightFactor = float3(0.0f, 0.0f, 0.0f);
    
        for (uint i = 0; i < lightData.directionalLightCount; i++)
            lightFactor += PhongDirectionalLight(lightData.directionalLights[i], cameraData.position, ray.Origin, normal);
        for (uint i = 0; i < lightData.pointLightCount; i++)
            lightFactor += PhongPointLight(lightData.pointLights[i], cameraData.position, ray.Origin, normal);
        
        if(innerPayload.hit == 0)
            payload.color = lightFactor * texColor.xyz;
        else
            payload.color = 0.5 * lightFactor * (innerPayload.color.xyz + texColor.xyz);
    }
    else
    {
        payload.color = texColor.xyz;
    }
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(2.4, 2.6, 2.2);
    payload.hit = 0;
}