#include "shader_common.hlsli"


ConstantBuffer<FrameData> frameData : register(b0);
ConstantBuffer<ObjectData> objData : register(b4);

float4 main(float4 pos:POSITION,float4 normal:NORMAL,float2 uv:TEXCOORD) : SV_Position
{
    float4 worldPos = mul(pos, objData.ModelMatrix);
    return mul(worldPos, frameData.ShadowViewPrjMatrix);
}