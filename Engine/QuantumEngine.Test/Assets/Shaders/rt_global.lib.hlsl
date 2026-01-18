#include "TransformStructs.hlsli"
#include "RTStructs.hlsli"

cbuffer _CameraData : register(b0)
{
    CameraData cameraData;
};

cbuffer ColorProperties : register(b1)
{
    float4 missColor;
    float4 hitColor;
};

cbuffer _RTProperties : register(b2)
{
    uint _missIndex;
};

RaytracingAccelerationStructure _RTScene : register(t1);
RWTexture2D<float4> _OutputTexture : register(u0);

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
    TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, _missIndex, 0, ray, payLoad);
    
    uint3 launchIndex = DispatchRaysIndex();
    _OutputTexture[launchIndex.xy] = float4(payLoad.color, 1);
}

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = hitColor.xyz;
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = missColor.xyz;
}
