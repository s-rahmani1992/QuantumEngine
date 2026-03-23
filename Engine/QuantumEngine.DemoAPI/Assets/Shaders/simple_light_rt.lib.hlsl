#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"
#include "Common/RTStructs.hlsli"

#ifdef _VULKAN

struct Properties
{
    float ambient;
    float diffuse;
    float specular;
    uint castShadow;
};

[[vk::push_constant]]
Properties Props;

#define ambient Props.ambient
#define diffuse Props.diffuse
#define specular Props.specular
#define castShadow Props.castShadow

#else

cbuffer MaterialProps : register(b0, space1)
{
    float ambient;
    float diffuse;
    float specular;
    uint castShadow = 0;
};

#endif

#ifdef _VULKAN
[[vk::binding(0, 1)]]
#endif
TRANSFORM_VAR_2(b1, space1)

#ifdef _VULKAN
[[vk::binding(1, 1)]]
#endif
CAMERA_VAR_2(b2, space1)

#ifdef _VULKAN
[[vk::binding(2, 1)]]
#endif
LIGHT_VAR_2(b3, space1)

#ifdef _VULKAN
[[vk::binding(3, 1)]]
#endif
Texture2D mainTexture : register(t0, space1);

#ifdef _VULKAN
[[vk::binding(5, 1)]]
#endif
sampler mainSampler : register(s0, space1);

#ifdef _VULKAN
[[vk::binding(6, 1)]]
#endif
RT_INDEX_BUFFER_VAR_2(t1, space1)

#ifdef _VULKAN
[[vk::binding(7, 1)]]
#endif
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