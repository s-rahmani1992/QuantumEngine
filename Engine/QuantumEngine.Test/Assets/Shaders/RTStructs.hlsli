struct GeneralPayload
{
    float3 color;
    uint recursionCount;
    uint targetMode; //0: start , 1: reflection  , 2: shadow
    float hit;
};

struct Vertex
{
    float3 pos;
    float2 uv;
    float3 normal;
};