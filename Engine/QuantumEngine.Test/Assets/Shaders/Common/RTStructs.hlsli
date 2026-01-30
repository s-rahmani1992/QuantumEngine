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

inline float3 Refract(float3 rayDirection, float3 normal, float refractionFactor, inout bool isReflect)
{
    float cosI = dot(normal, rayDirection);
    float refractionIndex = 1.0f;
        
    if (cosI < 0) // from outside to inside
        refractionIndex = 1.0f / refractionFactor;
    else // from inside to outside
        refractionIndex = refractionFactor;
        
    float sinT2 = refractionIndex * refractionIndex * (1.0 - cosI * cosI);
    isReflect = sinT2 > 1.0f;
    
    if (isReflect) // Total internal reflection
        return normalize(rayDirection - 2 * cosI * normal);
    else
        return normalize(refractionIndex * rayDirection + (refractionIndex * cosI - sqrt(1 - sinT2)) * normal);
}

#define RT_SCENE_VAR_1(t) \
RaytracingAccelerationStructure _RTScene : register(t);

#define RT_SCENE_VAR_2(t, space) \
RaytracingAccelerationStructure _RTScene : register(t, space);

#define RT_PROP_VAR_1(b) \
cbuffer _RTProperties : register(b) \
{ \
    uint _missIndex; \
};

#define RT_PROP_VAR_2(b, space) \
cbuffer _RTProperties : register(b, space) \
{ \
    uint _missIndex; \
};

#define RT_OUT_VAR_1(u) \
RWTexture2D<float4> _OutputTexture : register(u);

#define RT_OUT_VAR_2(u, space) \
RWTexture2D<float4> _OutputTexture : register(u, space);

#define RT_INDEX_BUFFER_VAR_1(t) \
StructuredBuffer<uint> _indexBuffer : register(t);

#define RT_INDEX_BUFFER_VAR_2(t, space) \
StructuredBuffer<uint> _indexBuffer : register(t, space);

#define RT_VERTEX_BUFFER_VAR_1(t) \
StructuredBuffer<Vertex> _vertexBuffer : register(t);

#define RT_VERTEX_BUFFER_VAR_2(t, space) \
StructuredBuffer<Vertex> _vertexBuffer : register(t, space);