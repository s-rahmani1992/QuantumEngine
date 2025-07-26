cbuffer MaterialProps : register(b0, space1)
{
    float4 color;
};

struct RayPayload
{
    float3 color;
};

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = color.xyz;
}