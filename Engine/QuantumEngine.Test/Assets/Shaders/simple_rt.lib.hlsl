#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"
#include "Common/RTStructs.hlsli"

TRANSFORM_VAR_2(b0, space1)

CAMERA_VAR_2(b1, space1)

Texture2D mainTexture : register(t0, space1);
sampler mainSampler : register(s0, space1);

RT_INDEX_BUFFER_VAR_2(t1, space1)

RT_VERTEX_BUFFER_VAR_2(t2, space1)

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    
    payload.color = texColor.xyz;
    
    if(payload.targetMode == 1)
        payload.hit = 1;
    else if (payload.targetMode == 2)
        payload.hit = 0;
}