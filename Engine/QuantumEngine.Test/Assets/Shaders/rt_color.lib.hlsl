#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer MaterialProps : register(b0, space1)
{
    float4 color;
    uint castShadow;
    float3 dummy;
};

cbuffer ObjectTransformData : register(b1, space1)
{
    TransformData transformData;
};

cbuffer CameraData : register(b2, space1)
{
    CameraData cameraData;
};

cbuffer LightData : register(b3, space1) {
    LightData lightData;
}

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
    float2 uv = v1.uv + attribs.barycentrics.x * (v2.uv - v1.uv) + attribs.barycentrics.y * (v3.uv - v1.uv);
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    
    float3 normal = v1.normal + attribs.barycentrics.x * (v2.normal - v1.normal) + attribs.barycentrics.y * (v3.normal - v1.normal);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float3 position = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    float3 lightFactor = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < lightData.directionalLightCount; i++)
        lightFactor += PhongDirectionalLight(lightData.directionalLights[i], cameraData.position, position, normal);
    for (uint i = 0; i < lightData.pointLightCount; i++)
        lightFactor += PhongPointLight(lightData.pointLights[i], cameraData.position, position, normal);
    
    payload.color = lightFactor * color.xyz * texColor.xyz;
    payload.hit = 1;
}