#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"

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
    float2 normPos : POSITION;
};

cbuffer CameraData : register(b1)
{
    CameraData cameraData;
};

cbuffer LightData : register(b3, space0)
{
    LightData lightData;
}

Texture2D<float4> gOutput : register(t0);
Texture2D<float4> gPositionTex : register(t1);
Texture2D<float4> gNormalTex : register(t2);

Texture2D mainTexture : register(t3);
sampler mainSampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float2 ndc = input.pos.xy / float2(1280, 720);
    
    float2 uv = float2(ndc.x, ndc.y);
    float3 pos = gPositionTex.SampleLevel(mainSampler, uv, 0).xyz;
    float3 norm = 2 * gNormalTex.SampleLevel(mainSampler, uv, 0).xyz - 1;
    float4 reflectionData = gOutput.Sample(mainSampler, uv, 0);
    float3 lightFactor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < lightData.directionalLightCount; i++)
        lightFactor += PhongDirectionalLight(lightData.directionalLights[i], cameraData.position, pos, norm);
    for (uint i = 0; i < lightData.pointLightCount; i++)
        lightFactor += PhongPointLight(lightData.pointLights[i], cameraData.position, pos, norm);
    
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    
    if (reflectionData.w < 0.1f)
        return float4(lightFactor * texColor.xyz, 1);
    else
        return float4(0.5 * lightFactor * (texColor.xyz + reflectionData.xyz), 1);
    //return float4(reflectionData.xyz, 1);
}