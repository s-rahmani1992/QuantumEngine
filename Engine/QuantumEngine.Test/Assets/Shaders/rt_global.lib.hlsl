#include "Common/TransformStructs.hlsli"
#include "Common/RTStructs.hlsli"

CAMERA_VAR_1(b0)

RT_PROP_VAR_1(b1)

cbuffer ColorProperties : register(b2)
{
    float4 missColor;
    float4 hitColor;
};

RT_SCENE_VAR_1(t0)

RT_OUT_VAR_1(u0)

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
    TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, payLoad);
    
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
