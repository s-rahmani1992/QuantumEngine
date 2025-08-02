struct Vertex
{
    float3 pos;
    float2 uv;
    float3 normal;
};

cbuffer MaterialProps : register(b0, space1)
{
    float4 color;
};

RaytracingAccelerationStructure gRtScene : register(t3, space1);

Texture2D mainTexture : register(t0, space1);
sampler mainSampler : register(s0, space1);

StructuredBuffer<uint> g_indices : register(t1, space1);
StructuredBuffer<Vertex> g_vertices : register(t2, space1);

struct RayPayload
{
    float3 color;
};

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint baseIndex = PrimitiveIndex() * 3;
    Vertex v1 = g_vertices[g_indices[baseIndex]];
    Vertex v2 = g_vertices[g_indices[baseIndex + 1]];
    Vertex v3 = g_vertices[g_indices[baseIndex + 2]];
    float3 rayDirection = WorldRayDirection();
    float3 normal = v1.normal + attribs.barycentrics.x * (v2.normal - v1.normal) + attribs.barycentrics.y * (v3.normal - v1.normal);
    float2 uv = v1.uv + attribs.barycentrics.x * (v2.uv - v1.uv) + attribs.barycentrics.y * (v3.uv - v1.uv);
    RayDesc ray;
    ray.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    ray.Direction = rayDirection - 2 * dot(normal, rayDirection) * normal;

    ray.TMin = 0.1;
    ray.TMax = 100000;

    RayPayload innerPayload;
    TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, innerPayload);


    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    payload.color = 0.5 * (innerPayload.color.xyz + texColor.xyz);
}