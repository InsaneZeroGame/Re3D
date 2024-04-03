#include "shader_common.hlsli"

ConstantBuffer<FrameData> frameData : register(b0);

PSInput main(float4 pos : POSITION,float4 normal : NORMAL,float2 textureCoord:TEXCOORD)
{
    PSInput input;
    input.position = mul(pos, frameData.ViewPrj);
    input.viewsSpacePos = mul(pos, frameData.View);
    input.normalViewSpace = mul(float4(normal.xyz, 0.0), frameData.NormalMatrix);
    input.UVCoord = textureCoord;
    input.color = float4(0.5f, 0.5f, 0.5f,1.0f);
    input.normal = normal.xyz;
    input.DirectionalLightDir = frameData.DirectionalLightDir.xyz;
    input.DirectionalLightColor = frameData.DirectionalLightColor.xyz;
    return input;
}