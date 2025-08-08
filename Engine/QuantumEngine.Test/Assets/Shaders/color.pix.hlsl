#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL; 
    float3 lightPos : POSITION;
};

cbuffer CameraData : register(b1)
{
    CameraData cameraData;
};

cbuffer Material : register(b2)
{
    float4 color;
};

cbuffer LightData : register(b3, space0) {
    LightData lightData;
}

Texture2D mainTexture : register(t0);
sampler mainSampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 lightFactor = float3(0.0f, 0.0f, 0.0f);
    
    for(uint i = 0; i < lightData.directionalLightCount; i++)
        lightFactor += PhongDirectionalLight(lightData.directionalLights[i], cameraData.position, input.lightPos, input.norm);
    for (uint i = 0; i < lightData.pointLightCount; i++)
        lightFactor += PhongPointLight(lightData.pointLights[i], cameraData.position, input.lightPos, input.norm);
    
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    return float4(lightFactor * texColor.xyz, 1);
}