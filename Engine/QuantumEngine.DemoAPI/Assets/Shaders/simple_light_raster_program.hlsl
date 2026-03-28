#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
    float3 worldPos : POSITION;
};


TRANSFORM_VAR_1(b0) 

CAMERA_VAR_1(b1) 

LIGHT_VAR_1(b2)

#ifdef _VULKAN
struct MaterialProperties
{
    float ambient;
    float diffuse;
    float specular;
    float textureFactor;
};

[[vk::push_constant]]
MaterialProperties MaterialProps;

#define ambient MaterialProps.ambient
#define diffuse MaterialProps.diffuse
#define specular MaterialProps.specular
#define textureFactor MaterialProps.textureFactor

#else

struct MaterialProperties
{
    float ambient;
    float diffuse;
    float specular;
    float textureFactor;
};

cbuffer MaterialProps : register(b3)
{
    MaterialProperties props;
};

#define ambient props.ambient
#define diffuse props.diffuse
#define specular props.specular
#define textureFactor props.textureFactor

#endif

#ifdef _VULKAN
[[vk::binding(3, 0)]]
#endif
Texture2D mainTexture : register(t0);

#ifdef _VULKAN
[[vk::binding(4, 0)]]
#endif
sampler mainSampler : register(s0);

VS_OUTPUT vs_main(VS_INPUT vertexIn)
{
    VS_OUTPUT vsOut;
    vsOut.pos = mul(float4(vertexIn.pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vsOut.texCoord = vertexIn.texCoord;
    vsOut.norm = mul(float4(vertexIn.norm, 1.0f), transformData.rotationMatrix).xyz;
    vsOut.worldPos = mul(float4(vertexIn.pos, 1.0f), transformData.modelMatrix).xyz;
    return vsOut;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET
{
    float3 ads = float3(ambient, diffuse, specular);
  
    float3 lightFactor = PhongLight(lightData, cameraData.position, input.worldPos, input.norm, ads);
    
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    return float4(lightFactor * texColor.xyz, 1);
}