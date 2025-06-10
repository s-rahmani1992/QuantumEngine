#include "colorRootSig.hlsli"

float3 color : register(b0);

[RootSignature(MyRS1)]
float4 main() : SV_TARGET
{
    return float4(color, 1.0f);
}