#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer MaterialProps : register(b0, space1)
{
    float ambient;
    float diffuse;
    float specular;
    float padding; // Padding
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
    float2 uv = CalculateUV(g_indices, g_vertices, attribs.barycentrics);
    float3 normal = CalculateNormal(g_indices, g_vertices, attribs.barycentrics);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);

    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, position, normal, ads);
    
    payload.color = lightFactor * texColor.xyz;
    payload.hit = 1;
}