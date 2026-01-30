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

#define TRANSFORM_VAR_1(b) \
cbuffer _ObjectTransformData : register(b) \
{ \
    TransformData transformData; \
};

#define TRANSFORM_VAR_2(b, space) \
cbuffer _ObjectTransformData : register(b, space) \
{ \
    TransformData transformData; \
};

#define CAMERA_VAR_1(b) \
cbuffer _CameraData : register(b) \
{ \
    CameraData cameraData; \
};

#define CAMERA_VAR_2(b, space) \
cbuffer _CameraData : register(b, space) \
{ \
    CameraData cameraData; \
};
