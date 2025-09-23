#include "TransformStructs.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
};

// Output structure for MRTs
struct PSOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    uint mask : SV_Target2;
};


PSOutput main(VS_OUTPUT input)
{
    PSOutput psOut;
    psOut.position = float4(input.worldPos, 1.0f);
    psOut.normal = float4(normalize(input.normal) * 0.5f + 0.5f, 1.0f); // Encode normal to [0,1] range
    psOut.mask = 1; // Example mask value, can be modified as needed
    
    return psOut;
}