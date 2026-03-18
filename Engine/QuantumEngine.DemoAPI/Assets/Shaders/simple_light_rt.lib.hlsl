#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"
#include "Common/RTStructs.hlsli"

cbuffer MaterialProps : register(b0, space1)
{
    float ambient;
    float diffuse;
    float specular;
    uint castShadow = 0;
};

TRANSFORM_VAR_2(b1, space1)

CAMERA_VAR_2(b2, space1)

LIGHT_VAR_2(b3, space1)

Texture2D mainTexture : register(t0, space1);
sampler mainSampler : register(s0, space1);

RT_INDEX_BUFFER_VAR_2(t1, space1)

RT_VERTEX_BUFFER_VAR_2(t2, space1)

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2 && castShadow > 0)
    {
        payload.hit = 1;
        return;
    }
    
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);

    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, position, normal, ads);
    
    payload.color = lightFactor * texColor.xyz;
    payload.hit = 1;
}