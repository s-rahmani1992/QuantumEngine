#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer CameraData : register(b0)
{
    CameraData cameraData;
};

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

float3 linearToSrgb(float3 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    

    float2 crd = float2(launchIndex.xy) + 0.5;
    float2 dims = float2(launchDim.xy);

    float2 screenPos = ((crd / dims) * 2.f - 1.f);
    screenPos.y = -screenPos.y;
    float4 worldPos = mul(float4(screenPos, 1.0f, 1.0f), cameraData.inverseProjectionMatrix);
    worldPos.xyz /= worldPos.w;
    RayDesc ray;
    ray.Origin = cameraData.position;
    ray.Direction = normalize(worldPos.xyz - ray.Origin);

    ray.TMin = 0;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 0;
    TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payLoad);
    //float3 col = linearToSrgb(payLoad.color);
    gOutput[launchIndex.xy] = float4(payLoad.color, 1);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.1f, 0.7f, 0.3f);
}
