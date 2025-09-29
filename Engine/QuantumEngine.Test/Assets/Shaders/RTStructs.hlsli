struct GeneralPayload
{
    float3 color;
    uint recursionCount;
    uint targetMode; //0: start , 1: reflection  , 2: shadow
    float hit;
};

struct Vertex
{
    float3 pos;
    float2 uv;
    float3 normal;
};

inline float3 CalculateScreenPosition(float4x4 inverseProjectMatrix)
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy) + 0.5;
    float2 dims = float2(launchDim.xy);

    float2 screenPos = ((crd / dims) * 2.f - 1.f);
    screenPos.y = -screenPos.y;
    float4 worldPos = mul(float4(screenPos, 1.0f, 1.0f), inverseProjectMatrix);
    worldPos.xyz /= worldPos.w;
    return worldPos.xyz;
}

inline float2 CalculateUV(StructuredBuffer<uint> indices, StructuredBuffer<Vertex> vertices, float2 barycentrics)
{
    uint baseIndex = PrimitiveIndex() * 3;
    Vertex v1 = vertices[indices[baseIndex]];
    Vertex v2 = vertices[indices[baseIndex + 1]];
    Vertex v3 = vertices[indices[baseIndex + 2]];
    return v1.uv + barycentrics.x * (v2.uv - v1.uv) + barycentrics.y * (v3.uv - v1.uv);
}

inline float3 CalculateNormal(StructuredBuffer<uint> indices, StructuredBuffer<Vertex> vertices, float2 barycentrics)
{
    uint baseIndex = PrimitiveIndex() * 3;
    Vertex v1 = vertices[indices[baseIndex]];
    Vertex v2 = vertices[indices[baseIndex + 1]];
    Vertex v3 = vertices[indices[baseIndex + 2]];
    return v1.normal + barycentrics.x * (v2.normal - v1.normal) + barycentrics.y * (v3.normal - v1.normal);
}

inline float3 CalculeteHitPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}