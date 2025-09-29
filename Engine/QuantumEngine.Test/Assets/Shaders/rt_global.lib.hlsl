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
    float3 pixelPos = CalculateScreenPosition(cameraData.inverseProjectionMatrix);
    
    RayDesc ray;
    ray.Origin = cameraData.position;
    ray.Direction = normalize(pixelPos - ray.Origin);
    ray.TMin = 0;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 0;
    TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payLoad);
    
    uint3 launchIndex = DispatchRaysIndex();
    gOutput[launchIndex.xy] = float4(payLoad.color, 1);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.1f, 0.7f, 0.3f);
}
