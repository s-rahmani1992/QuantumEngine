#include "colorRootSig.hlsli"
//#define MyRS1 \
//"RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT) " 
[RootSignature(MyRS1)]
float4 main(float3 pos : POSITION) : SV_POSITION
{
    return float4(pos, 1.0f);
}