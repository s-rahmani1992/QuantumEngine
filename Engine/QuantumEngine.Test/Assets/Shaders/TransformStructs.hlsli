struct TransformData
{
    float4x4 modelMatrix;
    float4x4 rotationMatrix;
    float4x4 modelViewMatrix;
};

struct CameraData
{
    float4x4 projectionMatrix;
    float4x4 inverseProjectionMatrix;
    float4x4 viewMatrix;
    float3 position;
};