#include "Common/TransformStructs.hlsli"
#include "Common/RTStructs.hlsli"

CAMERA_VAR_1(b0)

RT_PROP_VAR_1(b1)

RT_SCENE_VAR_1(t0)

RT_OUT_VAR_1(u0)

Texture2D<uint> _MaskTexture : register(t1);
Texture2D<float4> _PositionTexture : register(t2);
Texture2D<float4> _NormalTexture : register(t3);
sampler mainSampler : register(s0);

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    float2 uv = float2(launchIndex.xy) / float2(launchDim.xy);
    uint mask = _MaskTexture.Load(int3(launchIndex.xy, 0));
    
    if (mask == 0)
    {
        _OutputTexture[launchIndex.xy] = float4(0, 0, 0, 0);
        return;
    }
    
    float3 pos = _PositionTexture.SampleLevel(mainSampler, uv, 0).xyz;
    float3 norm = 2 * _NormalTexture.SampleLevel(mainSampler, uv, 0).xyz - 1;
    
    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = normalize(reflect(-cameraData.position + pos.xyz, norm));
    ray.TMin = 0.1;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 1;
    TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, payLoad);
    if(payLoad.hit == 0)
        _OutputTexture[launchIndex.xy] = float4(0.1f, 0.7f, 0.1f, 0.0f);
    else
        _OutputTexture[launchIndex.xy] = float4(payLoad.color, 1.0f);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.2f, 0.0f, 1.0f);
    payload.hit = 0;
}