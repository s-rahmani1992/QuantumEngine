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
    float3 norm : NORMAL;
    float3 worldPos : POSITION;
};

cbuffer _ObjectTransformData : register(b0)
{
    TransformData transformData;
};

cbuffer _CameraData : register(b1)
{
    CameraData cameraData;
};

cbuffer MaterialProps : register(b2)
{
    float ambient;
    float diffuse;
    float specular;
    float textureFactor;
};

cbuffer _LightData : register(b3)
{
    LightData lightData;
}

Texture2D mainTexture : register(t3);
Texture2D mainTexture1 : register(t4);
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
    
    float4 texColor = textureFactor * mainTexture.Sample(mainSampler, input.texCoord) + (1 - textureFactor) * mainTexture1.Sample(mainSampler, input.texCoord);
    return float4(lightFactor * texColor.xyz, 1);
}