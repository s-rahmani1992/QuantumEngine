#include "Common/RTStructs.hlsli"
#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"

cbuffer MissProps : register(b0, space1)
{
    uint castShadow;
    float ambient;
    float diffuse;
    float specular;
};

TRANSFORM_VAR_2(b1, space1)

CAMERA_VAR_2(b2, space1)

LIGHT_VAR_2(b3, space1)

RT_PROP_VAR_2(b4, space1)

RT_SCENE_VAR_2(t0, space1);

RT_INDEX_BUFFER_VAR_2(t1, space1)

RT_VERTEX_BUFFER_VAR_2(t2, space1)

Texture2D mainTexture : register(t3, space1);
sampler mainSampler : register(s0, space1);

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2 && castShadow > 0)
    {
        payload.hit = 1;
        return;
    }
    
    payload.hit = 1;
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 position = CalculeteHitPosition();
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    
    GeneralPayload innerPayload;
    innerPayload.targetMode = 2;
    innerPayload.recursionCount = payload.recursionCount + 1;
    
    float3 lightColor = float3(0.0f, 0.0f, 0.0f);
    float3 ads = float3(ambient, diffuse, specular);
    
    for (uint i = 0; i < lightData.directionalLightCount; i++)
    {
        DirectionalLight light = lightData.directionalLights[i];
        
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = -light.direction;
        ray.TMin = 0.1;
        ray.TMax = 100000;
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, innerPayload);

        if (innerPayload.hit > 0)
            lightColor += light.intensity * ambient * light.color.xyz;
        else
            lightColor += PhongDirectionalLight(light, cameraData.position, position, normal, ads);
    }
    
    for (uint i = 0; i < lightData.pointLightCount; i++)
    {
        PointLight light = lightData.pointLights[i];
        
        float d = distance(light.position, position);
        if (d > light.radius)
        {
            lightColor += ambient * light.color.xyz;
            continue;
        }
        
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = normalize(light.position - ray.Origin);
        ray.TMin = 0.1;
        ray.TMax = min(lightData.pointLights[i].radius, d);
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, innerPayload);

        if (innerPayload.hit > 0)
            lightColor += ambient * light.color.xyz;
        else
            lightColor += PhongPointLight(light, cameraData.position, position, normal, ads);
    }
    
    payload.color = lightColor * (texColor.xyz);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.hit = 0;
}