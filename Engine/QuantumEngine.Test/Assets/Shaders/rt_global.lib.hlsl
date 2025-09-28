#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer CameraData : register(b0)
{
    CameraData cameraData;
};

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

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

    gOutput[launchIndex.xy] = float4(payLoad.color, 1);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.1f, 0.7f, 0.3f);
}
