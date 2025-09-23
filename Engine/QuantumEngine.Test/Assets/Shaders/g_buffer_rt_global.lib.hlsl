#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer CameraData : register(b0)
{
    CameraData cameraData;
};

RaytracingAccelerationStructure gRtScene : register(t0);
Texture2D<uint> gMaskTex : register(t1);
Texture2D<float4> gPositionTex : register(t2);
Texture2D<float4> gNormalTex : register(t3);

RWTexture2D<float4> gOutput : register(u0);

sampler mainSampler : register(s0);

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    
    float2 uv = float2(launchIndex.xy) / float2(launchDim.xy);
    
    uint mask = gMaskTex.Load(int3(launchIndex.xy, 0));
    
    if (mask == 0)
    {
        gOutput[launchIndex.xy] = float4(0, 0, 0, 0);
        return;
    }
    
    float3 pos = gPositionTex.SampleLevel(mainSampler, uv, 0).xyz;
    float3 norm = 2 * gNormalTex.SampleLevel(mainSampler, uv, 0).xyz - 1;
    
    RayDesc ray;
    ray.Origin = pos;
    float3 rayDirection = normalize(pos.xyz - cameraData.position);
    ray.Direction = rayDirection - 2 * dot(norm, rayDirection) * norm;

    ray.TMin = 0;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 0;
    TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payLoad);
    //float3 col = linearToSrgb(payLoad.color);
    if(payLoad.hit == 0)
        gOutput[launchIndex.xy] = float4(0.1f, 0.7f, 0.1f, 0.0f);
    else
        gOutput[launchIndex.xy] = float4(payLoad.color, 1.0f);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.2f, 0.0f, 1.0f);
    payload.hit = 0;
}