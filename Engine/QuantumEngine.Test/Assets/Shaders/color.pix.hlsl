#include "colorRootSig.hlsli"

float4 color : register(b0);

[RootSignature(MyRS1)]
float4 main() : SV_TARGET
{
    return color;
}