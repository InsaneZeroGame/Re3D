#include "shader_common.hlsli"

cbuffer FrameData: register(b0)
{
    float4x4 ViewPrj;
    float4x4 View;
    float4x4 NormalMatrix;
    float4 DirectionalLightDir;
    float4 DirectionalLightColor;
};



PSInput main(float4 pos : POSITION,float4 normal : NORMAL,float4 color : COLOR,float2 textureCoord:TEXCOORD)
{
    PSInput input;
    input.position = mul(pos,ViewPrj);
    input.viewsSpacePos = mul(pos, View);
    input.normalViewSpace = mul(float4(normal.xyz, 0.0), NormalMatrix);
    input.UVCoord = textureCoord;
    input.color = color;
    input.normal = normal.xyz;
    input.DirectionalLightDir = DirectionalLightDir.xyz;
    input.DirectionalLightColor = DirectionalLightColor.xyz;
    return input;
}